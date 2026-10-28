param([string]$Out)
# Parse the clipboard T3D (Ctrl+C of graph nodes) and print each pin: node, name, bIsReference, bIsConst, LinkedTo count.
$t = Get-Clipboard -Raw
if ($Out) { Set-Content -Path $Out -Value $t -Encoding UTF8 }
$node = ''
foreach ($line in ($t -split "`r?`n")) {
  if ($line -match '^Begin Object Class=\S+ Name="([^"]+)"') { $node = $Matches[1] }
  if ($line -match 'CustomProperties Pin \((.*)\)\s*$') {
    $p = $Matches[1]
    $name = if ($p -match 'PinName="([^"]+)"') { $Matches[1] } else { '?' }
    $ref = if ($p -match 'PinType\.bIsReference=(\w+)') { $Matches[1] } else { '?' }
    $con = if ($p -match 'PinType\.bIsConst=(\w+)') { $Matches[1] } else { '?' }
    $sub = if ($p -match 'PinType\.PinSubCategoryObject=([^,]+)') { $Matches[1] } else { '' }
    $lk = if ($p -match 'LinkedTo=\(([^)]*)\)') { ($Matches[1].Split(',') | Where-Object { $_ -ne '' }).Count } else { 0 }
    "$node $name ref=$ref const=$con linked=$lk $sub"
  }
}
