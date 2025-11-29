"""
Utilities for working with Unreal Engine connections.

This module provides helper functions for common operations with Unreal Engine connections.
"""

import logging
import socket
import json
from typing import Dict, List, Any, Optional

# Get logger
logger = logging.getLogger("UnrealMCP")

# Configuration
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557

class UnrealConnection:
    """Connection to an Unreal Engine instance."""
    
    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False
    
    def connect(self) -> bool:
        """Connect to the Unreal Engine instance."""
        try:
            # Close any existing socket
            if self.socket:
                try:
                    self.socket.close()
                except:
                    pass
                self.socket = None
            
            logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(8)  # 30 second timeout for complex operations
            
            # Set socket options for better stability
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            
            # Set larger buffer sizes
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)
            
            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            logger.info("Connected to Unreal Engine")
            return True
            
        except socket.timeout:
            logger.error(f"Connection timeout to Unreal Engine at {UNREAL_HOST}:{UNREAL_PORT}")
            self.connected = False
            return False
        except ConnectionRefusedError:
            logger.error(f"Connection refused by Unreal Engine at {UNREAL_HOST}:{UNREAL_PORT} - is Unreal Engine running?")
            self.connected = False
            return False
        except Exception as e:
            logger.error(f"Failed to connect to Unreal: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the Unreal Engine instance."""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.socket = None
        self.connected = False

    def receive_full_response(self, sock, buffer_size=4096) -> bytes:
        """Receive a complete response from Unreal, handling chunked data."""
        chunks = []
        sock.settimeout(30)  # 30 second timeout for complex operations
        debug_log_path = "E:/code/unreal-mcp/Python/tcp_debug.log"
        
        try:
            import datetime
            with open(debug_log_path, "a", encoding="utf-8") as f:
                f.write(f"\n\n=== RECEIVE START: {datetime.datetime.now()} ===\n")
            
            while True:
                try:
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"Calling sock.recv({buffer_size})...\n")
                    
                    chunk = sock.recv(buffer_size)
                    
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"Received chunk: {len(chunk) if chunk else 0} bytes\n")
                    
                    if not chunk:
                        if not chunks:
                            with open(debug_log_path, "a", encoding="utf-8") as f:
                                f.write("ERROR: Connection closed before receiving data\n")
                            raise Exception("Connection closed before receiving data")
                        with open(debug_log_path, "a", encoding="utf-8") as f:
                            f.write("No more chunks, breaking...\n")
                        break
                    chunks.append(chunk)
                    
                    # Process the data received so far
                    data = b''.join(chunks)
                    decoded_data = data.decode('utf-8')
                    
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"Total data so far: {len(data)} bytes\n")
                        f.write(f"Decoded preview: {decoded_data[:200]}...\n")
                    
                    # Try to parse as JSON to check if complete
                    try:
                        json.loads(decoded_data)
                        logger.info(f"Received complete response ({len(data)} bytes)")
                        with open(debug_log_path, "a", encoding="utf-8") as f:
                            f.write(f"SUCCESS: Complete JSON received!\n")
                            f.write(f"Full response: {decoded_data}\n")
                        return data
                    except json.JSONDecodeError as je:
                        # Not complete JSON yet, continue reading
                        logger.debug(f"Received partial response, waiting for more data...")
                        with open(debug_log_path, "a", encoding="utf-8") as f:
                            f.write(f"Partial JSON (error: {je}), continuing...\n")
                        continue
                    except Exception as e:
                        logger.warning(f"Error processing response chunk: {str(e)}")
                        with open(debug_log_path, "a", encoding="utf-8") as f:
                            f.write(f"ERROR processing chunk: {e}\n")
                        continue
                        
                except socket.timeout as st:
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"SOCKET TIMEOUT in recv loop after {len(chunks)} chunks\n")
                    raise st
                except Exception as ex:
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"EXCEPTION in recv loop: {ex}\n")
                    raise ex
                    
        except socket.timeout:
            logger.warning(f"Socket timeout during receive after {len(chunks)} chunks")
            with open(debug_log_path, "a", encoding="utf-8") as f:
                f.write(f"TIMEOUT: After {len(chunks)} chunks\n")
            if chunks:
                # If we have some data already, try to use it
                data = b''.join(chunks)
                logger.info(f"Received {len(data)} bytes before timeout")
                with open(debug_log_path, "a", encoding="utf-8") as f:
                    f.write(f"Attempting to use partial data: {len(data)} bytes\n")
                    f.write(f"Partial data: {data.decode('utf-8', errors='replace')}\n")
                try:
                    json.loads(data.decode('utf-8'))
                    logger.info(f"Using partial response after timeout ({len(data)} bytes)")
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"SUCCESS: Partial data is valid JSON!\n")
                    return data
                except Exception as parse_error:
                    logger.warning(f"Partial data is not valid JSON: {parse_error}")
                    with open(debug_log_path, "a", encoding="utf-8") as f:
                        f.write(f"ERROR: Partial data is not valid JSON: {parse_error}\n")
            raise Exception(f"Timeout receiving Unreal response after 30 seconds (received {len(chunks)} chunks)")
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            with open(debug_log_path, "a", encoding="utf-8") as f:
                f.write(f"FATAL ERROR: {e}\n")
            raise
    
    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        """Send a command to Unreal Engine and get the response."""
        # Always reconnect for each command, since Unreal closes the connection after each command
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
        
        if not self.connect():
            logger.error("Failed to connect to Unreal Engine for command")
            return None
        
        try:
            # Match Unity's command format exactly
            command_obj = {
                "type": command,  # Use "type" instead of "command"
                "params": params or {}  # Use Unity's params or {} pattern
            }
            
            # Send without newline, exactly like Unity
            command_json = json.dumps(command_obj)
            logger.info(f"Sending command: {command_json}")
            self.socket.sendall(command_json.encode('utf-8'))
            
            # Read response using improved handler
            response_data = self.receive_full_response(self.socket)
            response = json.loads(response_data.decode('utf-8'))
            
            # Log complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Always close the connection after command is complete
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
            
            return response
            
        except Exception as e:
            logger.error(f"Error sending command: {e}")
            # Always reset connection state on any error
            self.connected = False
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            return {
                "status": "error",
                "error": str(e)
            }

# Global connection state
_unreal_connection: UnrealConnection = None

def get_unreal_engine_connection():
    """Get a connection to Unreal Engine."""
    global _unreal_connection
    try:
        if _unreal_connection is None:
            _unreal_connection = UnrealConnection()
        return _unreal_connection
    except Exception as e:
        logger.error(f"Error getting Unreal connection: {e}")
        return None

def send_unreal_command(command_name: str, params: Dict[str, Any]) -> Dict[str, Any]:
    """Send a command to Unreal Engine with proper error handling."""
    try:
        unreal = get_unreal_engine_connection()
        if not unreal:
            return {"status": "error", "error": "Failed to connect to Unreal Engine"}
        
        logger.info(f"Sending command '{command_name}' with params: {params}")
        response = unreal.send_command(command_name, params)
        
        if not response:
            logger.error(f"No response from Unreal Engine for command '{command_name}'")
            return {"status": "error", "error": "No response from Unreal Engine"}
        
        logger.info(f"Command '{command_name}' response: {response}")
        
        # Handle nested error objects from C++ MCPErrorHandler
        if response.get("success") is False:
            error_field = response.get("error")
            error_message = "Unknown Unreal error"
            
            if isinstance(error_field, dict):
                # This is a nested error object from C++ MCPErrorHandler
                # Extract the actual error message from the nested structure
                error_message = (error_field.get("errorMessage") or 
                               error_field.get("errorDetails") or 
                               error_field.get("message") or 
                               "Unknown nested error")
                logger.error(f"Unreal nested error: {error_message}")
            elif isinstance(error_field, str):
                # Simple string error message
                error_message = error_field
                logger.error(f"Unreal string error: {error_message}")
            else:
                # Fallback to message field or default
                error_message = response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal fallback error: {error_message}")
            
            # Convert to standard error format
            return {
                "status": "error",
                "error": error_message
            }
        
        return response
        
    except Exception as e:
        error_msg = f"Error executing Unreal command '{command_name}': {e}"
        logger.error(error_msg)
        return {"status": "error", "error": error_msg}