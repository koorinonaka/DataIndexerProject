param([string]$Png, [int]$Y = 970, [int]$X0 = 40, [int]$X1 = 1325)
# Measure horizontal scrollbar thumb: longest run of bright pixels on row Y (window-capture coords).
Add-Type -AssemblyName System.Drawing
$b = [System.Drawing.Bitmap]::new($Png)
$best = 0; $bs = 0; $run = 0; $rs = 0
for ($x = $X0; $x -lt $X1; $x++) {
  $c = $b.GetPixel($x, $Y); $l = ($c.R + $c.G + $c.B) / 3
  if ($l -gt 60) { if ($run -eq 0) { $rs = $x }; $run++; if ($run -gt $best) { $best = $run; $bs = $rs } } else { $run = 0 }
}
$b.Dispose()
$track = 1281
"thumb start=$bs len=$best -> content width ~ $([int]($track * $track / [Math]::Max($best,1)))"
