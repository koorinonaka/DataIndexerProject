param([string]$Points)  # "x,y;x,y;..." real OS left clicks, each with arrive+dwell
if (-not ([System.Management.Automation.PSTypeName]'K7').Type) {
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public class K7{
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint dx,uint dy,uint d,UIntPtr e);
 [DllImport("user32.dll")] public static extern void keybd_event(byte vk,byte sc,uint f,UIntPtr e);}
'@
}
foreach ($pt in $Points.Split(';')) {
  if ($pt.StartsWith('h:')) { $x, $y = $pt.Substring(2).Split(',') | ForEach-Object { [int]$_ }; [K7]::SetCursorPos($x, $y) | Out-Null; Start-Sleep -Milliseconds 700; continue }
  $vk = 0; $p = $pt
  if ($pt.StartsWith('c:')) { $vk = 0x11; $p = $pt.Substring(2) } elseif ($pt.StartsWith('s:')) { $vk = 0x10; $p = $pt.Substring(2) }
  $x, $y = $p.Split(',') | ForEach-Object { [int]$_ }
  [K7]::SetCursorPos($x - 10, $y) | Out-Null; Start-Sleep -Milliseconds 150
  [K7]::SetCursorPos($x, $y) | Out-Null; Start-Sleep -Milliseconds 300
  if ($vk) { [K7]::keybd_event($vk, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 100 }
  [K7]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 80
  [K7]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 100
  if ($vk) { [K7]::keybd_event($vk, 0, 2, [UIntPtr]::Zero) }
  Start-Sleep -Milliseconds 400
}
"clicked $Points"
