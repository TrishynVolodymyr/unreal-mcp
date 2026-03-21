# IFS (Iterated Function System) Fern/Leaf Reference

Collected coefficient sets for generating fractal plant textures via chaos game algorithm.

## How IFS Works

Each transform is an affine function: `x' = a*x + b*y + e`, `y' = c*x + d*y + f`
- Pick a random transform weighted by probability `p`
- Apply it to current point (x, y)
- Plot the new point
- Repeat millions of times

### Transform Roles (typical 4-transform fern)
| # | Role | Probability | What it controls |
|---|------|-------------|-----------------|
| T1 | Stem | 1-2% | Base/root point |
| T2 | Body | 84-85% | Overall shape, self-similarity, aspect ratio |
| T3 | Left pinnae | 7% | Left frond angle, length, curvature |
| T4 | Right pinnae | 7% | Right frond angle, length, curvature |

### Key Shape Controls
- **T2 `a` and `d`**: aspect ratio/taper. Lower `d` = fewer pinnae rows (squatter). Lower `a` = narrower.
- **T2 `b` and `c`**: stem curvature. Non-zero = curved/leaning fern.
- **T3/T4 `b`**: pinnae length (larger magnitude = longer)
- **T3/T4 `c`**: pinnae angle (larger = steeper upward angle)
- **T3/T4 probabilities**: asymmetric values = one side denser than other

## Coefficient Sets

Format: `(a, b, c, d, e, f, probability)`

### Classic Barnsley Fern
The iconic fractal fern. Curved stem, large pinnae with visible sub-pinnae.
```
(0.0,   0.0,   0.0,   0.16,  0.0,   0.0,   0.01)
(0.85,  0.04, -0.04,  0.85,  0.0,   1.6,   0.85)
(0.2,  -0.26,  0.23,  0.22,  0.0,   1.6,   0.07)
(-0.15, 0.28,  0.26,  0.24,  0.0,   0.44,  0.07)
```
Source: Barnsley, "Fractals Everywhere" (1988)

### Cyclosorus (Confirmed_02 coefficients)
Narrow tall fern, many small pinnae, symmetric. Clean anime-friendly look.
```
(0.0,   0.0,   0.0,   0.25,  0.0,  -0.14,  0.02)
(0.95,  0.0,   0.0,   0.93,  0.0,   0.5,   0.84)
(0.04, -0.18,  0.16,  0.045,-0.08,  0.02,  0.07)
(-0.04, 0.18,  0.16,  0.045, 0.08,  0.12,  0.07)
```
Source: chradams.co.uk fern generator

### Curved Arch (Confirmed_01 coefficients)
Diamond/rhombus shape, stem curves right. Asymmetric pinnae probabilities.
```
(0.0,   0.0,   0.0,   0.20,  0.0,  -0.12,  0.01)
(0.93,  0.03, -0.03,  0.90,  0.0,   0.55,  0.84)
(0.05, -0.22,  0.18,  0.05, -0.08,  0.02,  0.09)
(-0.05, 0.22,  0.18,  0.05,  0.08,  0.12,  0.06)
```
Key: `b=0.03, c=-0.03` in T2 creates the curve.

### Fishbone
Skeletal fern with tiny symmetric pinnae. Very thin spine. Clean silhouette.
```
(0.0,   0.0,   0.0,   0.25,  0.0,  -0.4,   0.02)
(0.95,  0.002,-0.002, 0.93, -0.002, 0.5,   0.84)
(0.035,-0.11,  0.27,  0.01, -0.05,  0.005, 0.07)
(-0.04, 0.11,  0.27,  0.01,  0.047, 0.06,  0.07)
```
Source: chradams.co.uk fern generator

### Culcita (Tree Fern)
Broad round shape, wide-angled pinnae with visible sub-fronds.
```
(0.0,   0.0,   0.0,   0.25,  0.0,  -0.14,  0.02)
(0.85,  0.02, -0.02,  0.83,  0.0,   1.0,   0.84)
(0.09, -0.28,  0.3,   0.11,  0.0,   0.6,   0.07)
(-0.09, 0.28,  0.3,   0.09,  0.0,   0.7,   0.07)
```
Source: chradams.co.uk fern generator

### Modified Barnsley
Shorter, wider pinnae angles. Squat shape with big lower fronds.
```
(0.0,   0.0,   0.0,   0.2,   0.0,  -0.12,  0.01)
(0.845, 0.035,-0.035, 0.82,  0.0,   1.6,   0.85)
(0.2,  -0.31,  0.255, 0.245, 0.0,   0.29,  0.07)
(-0.15, 0.24,  0.25,  0.20,  0.0,   0.68,  0.07)
```
Source: chradams.co.uk fern generator

### Paul Bourke Leaf IFS
Multi-lobed leaf, NOT a fern. Diagonal branching structure.
Probabilities not in original — estimated from attractor density.
```
(0.0,    0.2439,  0.0,    0.3053, 0.0,    0.0,    0.10)
(0.7248, 0.0337, -0.0253, 0.7426, 0.206,  0.2538, 0.45)
(0.1583,-0.1297,  0.355,  0.3676, 0.1383, 0.175,  0.22)
(0.3386, 0.3694,  0.2227,-0.0756, 0.0679, 0.0826, 0.23)
```
Source: paulbourke.net/fractals/ifs/

### 1999 Variant (3 transforms)
Asymmetric spiral with curling tendrils. Very different structure.
Only 3 transforms — unusual.
```
(0.01, -0.41,  0.39,  0.0, -0.28, -0.185, 0.10)
(0.7,   0.33, -0.35,  0.7,  0.185, 0.015, 0.70)
(0.0,   0.175, 0.013, 0.46,-0.095,-0.285, 0.20)
```
Source: paulbourke.net/fractals/ifs/

### Twig (3 transforms)
Asymmetric branching twig. Not fern-like but good for dead branch textures.
```
(0.387, 0.430,  0.430, -0.387, 0.2560, 0.5220, 0.333)
(0.441,-0.091, -0.009, -0.322, 0.4219, 0.5059, 0.333)
(-0.468,0.020, -0.113,  0.015, 0.4,    0.4,    0.334)
```
Source: paulbourke.net/fractals/ifs/

### Windswept Tree (5 transforms)
Asymmetric canopy with wind-blown silhouette. Very sparse.
```
(0.195, -0.488, 0.344,  0.443, 0.4431, 0.2452, 0.2)
(0.462,  0.414,-0.252,  0.361, 0.2511, 0.5692, 0.2)
(-0.637, 0.000, 0.000,  0.501, 0.8562, 0.2512, 0.2)
(-0.035, 0.070,-0.469,  0.022, 0.4884, 0.5069, 0.2)
(-0.058,-0.070, 0.453, -0.111, 0.5976, 0.0969, 0.2)
```
Source: paulbourke.net/fractals/ifs/

### Symmetric Binary Tree (4 transforms)
Clean Y-shaped branching tree. Equal probabilities.
```
(0.01,  0.00,  0.00,  0.45,  0.00, 0.00, 0.25)
(-0.01, 0.00,  0.00, -0.45,  0.00, 0.40, 0.25)
(0.42, -0.42,  0.42,  0.42,  0.00, 0.40, 0.25)
(0.42,  0.42, -0.42,  0.42,  0.00, 0.40, 0.25)
```
Source: paulbourke.net/fractals/ifs/

### Complex Multi-Branch Tree (7 transforms)
Detailed multi-branch tree. Equal probabilities (1/7 each).
```
(0.05,  0.00,  0.00,  0.40, -0.06, -0.47, 0.143)
(-0.05, 0.00,  0.00, -0.40, -0.06, -0.47, 0.143)
(0.03, -0.14,  0.00,  0.26, -0.16, -0.01, 0.143)
(-0.03, 0.14,  0.00, -0.26, -0.16, -0.01, 0.143)
(0.56,  0.44, -0.37,  0.51,  0.30,  0.15, 0.143)
(0.19,  0.07, -0.10,  0.15, -0.20,  0.28, 0.143)
(-0.33,-0.34, -0.33,  0.34, -0.54,  0.39, 0.143)
```
Source: paulbourke.net/fractals/ifs/

### Maple Leaf (4 transforms)
Broad multi-lobed maple leaf. Equal probabilities.
```
(0.14,   0.01,   0.00,  0.51, -0.08, -1.31, 0.25)
(0.43,   0.52,  -0.45,  0.50,  1.49, -0.75, 0.25)
(0.45,  -0.49,   0.47,  0.47, -1.62, -0.74, 0.25)
(0.49,   0.00,   0.00,  0.51,  0.02,  1.62, 0.25)
```
Source: paulbourke.net/fractals/ifs/

## Generation Parameters (from our pipeline)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Resolution | 1024x1024 | Grayscale + alpha PNG |
| Iterations | 4,000,000 | More = denser fill |
| Gaussian sigma | 0.7 | Lower = sharper pinnae, higher = merged |
| Alpha threshold | 25 | Below this → fully transparent |
| Min cluster size | 50px | Smaller clusters removed (stray dots) |
| Padding | 30px | Border around fern |

## Quality Validation Checks
1. **Top merge**: top 12% of image, no row should have >25% fill (prevents blob at tip)
2. **Fill ratio**: 3-30% (too sparse = wireframe, too dense = solid blob)
3. **Isolated clusters**: remove connected components < 50px (stray dots from stem transform)

## Sources
- Paul Bourke: paulbourke.net/fractals/ifs/
- Barnsley Fern Generator: chradams.co.uk/fern/maker.html
- Algorithm Archive: algorithm-archive.org (Barnsley fern chapter)
- Barnsley, "Fractals Everywhere" (1988) — original publication
