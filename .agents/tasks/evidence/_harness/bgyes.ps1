param([string]$Evidence, [int]$WaitSec = 4, [int]$TimeoutSec = 40)
# Background helper: wait for a modal UE message dialog, save a screenshot, then click its blue primary ("はい") button.
Add-Type -AssemblyName System.Drawing, System.Windows.Forms
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public class B9{
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint dx,uint dy,uint d,UIntPtr e);}
'@
Start-Sleep -Seconds $WaitSec
$deadline = (Get-Date).AddSeconds($TimeoutSec)
while ((Get-Date) -lt $deadline) {
  $bmp = New-Object System.Drawing.Bitmap 1920, 1080
  $g = [System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen(0, 0, 0, 0, $bmp.Size); $g.Dispose()
  # primary button blue ~ (0,112,224); scan the centre region only
  $n = 0; $sx = 0; $sy = 0
  for ($y = 300; $y -lt 780; $y += 2) { for ($x = 560; $x -lt 1360; $x += 2) {
    $c = $bmp.GetPixel($x, $y)
    if ($c.B -gt 190 -and $c.R -lt 40 -and $c.G -gt 80 -and $c.G -lt 150) { $n++; $sx += $x; $sy += $y }
  } }
  if ($n -gt 150) {
    if ($Evidence) { $bmp.Save($Evidence, [System.Drawing.Imaging.ImageFormat]::Png) }
    $bmp.Dispose()
    $cx = [int]($sx / $n); $cy = [int]($sy / $n)
    [B9]::SetCursorPos($cx - 10, $cy) | Out-Null; Start-Sleep -Milliseconds 200
    [B9]::SetCursorPos($cx, $cy) | Out-Null; Start-Sleep -Milliseconds 300
    [B9]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 80; [B9]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
    "clicked blue button at $cx,$cy (px=$n)"; exit 0
  }
  $bmp.Dispose(); Start-Sleep -Milliseconds 700
}
"no dialog found"
