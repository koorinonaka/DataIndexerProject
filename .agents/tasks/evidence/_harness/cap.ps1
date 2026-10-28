param([string]$Out, [int]$L=0, [int]$T=0, [int]$R=0, [int]$B=0, [switch]$Desktop)
# Capture the largest visible UnrealEditor window via PrintWindow (works while occluded), or the
# whole virtual desktop with -Desktop (needed for popup menus / separate windows). Optional crop.
Add-Type -AssemblyName System.Drawing
if (-not ([System.Management.Automation.PSTypeName]'W4').Type) {
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;using System.Collections.Generic;
[StructLayout(LayoutKind.Sequential)] public struct RECT4{public int L,T,R,B;}
public class W4{
 public delegate bool EP(IntPtr h,IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EP f,IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint p);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h,IntPtr hdc,uint f);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h,out RECT4 r);
 public static IntPtr Largest(uint pid){IntPtr best=IntPtr.Zero;long area=0;
  EnumWindows((h,l)=>{uint p;GetWindowThreadProcessId(h,out p);if(p==pid&&IsWindowVisible(h)){RECT4 r;GetWindowRect(h,out r);long a=(long)(r.R-r.L)*(r.B-r.T);if(a>area){area=a;best=h;}}return true;},IntPtr.Zero);return best;}}
'@
}
if ($Desktop) {
  Add-Type -AssemblyName System.Windows.Forms
  $vb = [System.Windows.Forms.SystemInformation]::VirtualScreen
  $bmp = New-Object System.Drawing.Bitmap $vb.Width, $vb.Height
  $g = [System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($vb.Location, [System.Drawing.Point]::Empty, $vb.Size); $g.Dispose()
  $info = "desktop $($vb.Width)x$($vb.Height) at $($vb.X),$($vb.Y)"
} else {
  $p = Get-Process UnrealEditor | Select-Object -First 1
  $hw = [W4]::Largest([uint32]$p.Id)
  $wr = New-Object RECT4
  [void][W4]::GetWindowRect($hw, [ref]$wr)
  $w = $wr.R - $wr.L; $h = $wr.B - $wr.T
  $bmp = New-Object System.Drawing.Bitmap $w, $h
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc(); [void][W4]::PrintWindow($hw, $hdc, 2); $g.ReleaseHdc($hdc); $g.Dispose()
  $info = "window ${w}x${h} at $($wr.L),$($wr.T)"
}
New-Item -ItemType Directory -Force -Path (Split-Path $Out) | Out-Null
if ($R -gt $L -and $B -gt $T) {
  $c = $bmp.Clone((New-Object System.Drawing.Rectangle $L, $T, ($R-$L), ($B-$T)), $bmp.PixelFormat)
  $c.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png); $c.Dispose()
} else { $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png) }
$bmp.Dispose()
"saved $Out ($info)"
