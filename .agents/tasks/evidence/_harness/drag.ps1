param([int]$X1, [int]$Y1, [int]$X2, [int]$Y2, [int]$Steps = 20)
# Real OS mouse drag: arrive, dwell, press, threshold moves, slow travel, dwell, release.
if (-not ([System.Management.Automation.PSTypeName]'M5').Type) {
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public class M5{
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint dx,uint dy,uint d,UIntPtr e);
 [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint p);
 [DllImport("user32.dll")] public static extern IntPtr GetAncestor(IntPtr h,uint f);
 [StructLayout(LayoutKind.Sequential)] public struct POINT{public int X,Y;}
 public static uint PidAt(int x,int y){POINT p;p.X=x;p.Y=y;IntPtr h=GetAncestor(WindowFromPoint(p),2);uint pid;GetWindowThreadProcessId(h,out pid);return pid;}}
'@
}
$ue = (Get-Process UnrealEditor | Select-Object -First 1).Id
$pid0 = [M5]::PidAt($X1, $Y1)
if ($pid0 -ne $ue) { "ABORT: window at $X1,$Y1 belongs to pid $pid0, not UnrealEditor $ue"; exit 1 }
[M5]::SetCursorPos($X1 - 30, $Y1 + 5) | Out-Null; Start-Sleep -Milliseconds 150
[M5]::SetCursorPos($X1, $Y1) | Out-Null; Start-Sleep -Milliseconds 300
[M5]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 150
for ($i = 1; $i -le $Steps; $i++) {
  $x = [int]($X1 + ($X2 - $X1) * $i / $Steps); $y = [int]($Y1 + ($Y2 - $Y1) * $i / $Steps)
  [M5]::SetCursorPos($x, $y) | Out-Null; Start-Sleep -Milliseconds 30
}
Start-Sleep -Milliseconds 300
[M5]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 300
"dragged $X1,$Y1 -> $X2,$Y2"
