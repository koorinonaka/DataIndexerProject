param([int]$X, [int]$Y)
# Move the real cursor to X,Y (arrive from 10px left, dwell) and report whether the OS cursor is IDC_SIZENS / IDC_ARROW / other.
if (-not ([System.Management.Automation.PSTypeName]'C8').Type) {
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public class C8{
 [StructLayout(LayoutKind.Sequential)] public struct CI{public int cb;public int flags;public IntPtr h;public int x;public int y;}
 [DllImport("user32.dll")] public static extern bool GetCursorInfo(ref CI c);
 [DllImport("user32.dll")] public static extern IntPtr LoadCursor(IntPtr i,int id);
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);}
'@
}
[C8]::SetCursorPos($X - 10, $Y) | Out-Null; Start-Sleep -Milliseconds 300
[C8]::SetCursorPos($X, $Y) | Out-Null; Start-Sleep -Milliseconds 600
$c = New-Object C8+CI; $c.cb = [Runtime.InteropServices.Marshal]::SizeOf($c); [void][C8]::GetCursorInfo([ref]$c)
$n = @{ 32645 = 'SIZENS'; 32512 = 'ARROW'; 32513 = 'IBEAM'; 32649 = 'HAND'; 32644 = 'SIZEWE'; 32646 = 'SIZEALL' }
$name = 'other'; foreach ($k in $n.Keys) { if ([C8]::LoadCursor([IntPtr]::Zero, $k) -eq $c.h) { $name = $n[$k] } }
"cursor at $X,$Y = $name"
