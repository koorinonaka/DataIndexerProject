param([int]$X1, [int]$Y1, [int]$X2, [int]$Y2, [string]$Out, [int]$Steps = 40, [int]$L = 0, [int]$T = 0, [int]$R = 1920, [int]$B = 1080)
# Real OS mouse drag that captures the desktop (CopyFromScreen, so drag decorators show) while still holding, then releases.
Add-Type -AssemblyName System.Drawing
if (-not ([System.Management.Automation.PSTypeName]'M6').Type) {
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public class M6{
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint dx,uint dy,uint d,UIntPtr e);}
'@
}
[M6]::SetCursorPos($X1 - 20, $Y1 + 3) | Out-Null; Start-Sleep -Milliseconds 200
[M6]::SetCursorPos($X1, $Y1) | Out-Null; Start-Sleep -Milliseconds 400
[M6]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 200
for ($d = 1; $d -le 6; $d++) { [M6]::SetCursorPos($X1 + $d * [Math]::Sign($X2 - $X1 + 0.1) * 2, $Y1 + $d * [Math]::Sign($Y2 - $Y1) * 2) | Out-Null; Start-Sleep -Milliseconds 40 }
for ($i = 1; $i -le $Steps; $i++) {
  $x = [int]($X1 + ($X2 - $X1) * $i / $Steps); $y = [int]($Y1 + ($Y2 - $Y1) * $i / $Steps)
  [M6]::SetCursorPos($x, $y) | Out-Null; Start-Sleep -Milliseconds 40
}
Start-Sleep -Milliseconds 600
if ($Out) {
  $bmp = New-Object System.Drawing.Bitmap ($R - $L), ($B - $T)
  $g = [System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($L, $T, 0, 0, $bmp.Size); $g.Dispose()
  $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
}
[M6]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 400
"dragged $X1,$Y1 -> $X2,$Y2 (captured while held: $Out)"
