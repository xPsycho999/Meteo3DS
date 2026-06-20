# Regenerates cia/banner.png (256x128) — the Home-Menu banner shown when hovering
# the title. Polished flat design: sky gradient, sun with glow + rays, two-tone
# cloud, dark bottom gradient for text legibility, title + subtitle.
#   powershell -ExecutionPolicy Bypass -File cia\make-banner.ps1
Add-Type -AssemblyName System.Drawing
$W = 256; $H = 128
$bmp = New-Object System.Drawing.Bitmap($W, $H)
$g   = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode     = 'AntiAlias'
$g.TextRenderingHint = 'AntiAliasGridFit'

# --- sky gradient -----------------------------------------------------------
$skyTop = [System.Drawing.Color]::FromArgb(47,134,214)
$skyBot = [System.Drawing.Color]::FromArgb(133,191,234)
$rect   = New-Object System.Drawing.Rectangle(0,0,$W,$H)
$sky    = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect,$skyTop,$skyBot,90)
$g.FillRectangle($sky, $rect)

# --- sun with soft glow + rays ---------------------------------------------
$cx = 58; $cy = 44
$glow = New-Object System.Drawing.Drawing2D.GraphicsPath
$glow.AddEllipse($cx-34,$cy-34,68,68)
$pgb = New-Object System.Drawing.Drawing2D.PathGradientBrush($glow)
$pgb.CenterColor    = [System.Drawing.Color]::FromArgb(150,255,236,170)
$pgb.SurroundColors = @([System.Drawing.Color]::FromArgb(0,255,236,170))
$g.FillPath($pgb, $glow)

$rayPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(235,255,214,92), 4)
$rayPen.StartCap = 'Round'; $rayPen.EndCap = 'Round'
for ($i = 0; $i -lt 8; $i++) {
    $a = $i * [Math]::PI / 4
    $x1 = $cx + [Math]::Cos($a) * 24; $y1 = $cy + [Math]::Sin($a) * 24
    $x2 = $cx + [Math]::Cos($a) * 33; $y2 = $cy + [Math]::Sin($a) * 33
    $g.DrawLine($rayPen, $x1, $y1, $x2, $y2)
}
$sunBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255,255,205,66))
$g.FillEllipse($sunBrush, $cx-19, $cy-19, 38, 38)

# --- two-tone cloud ---------------------------------------------------------
function Cloud($ox,$oy,$s,$col){
    $b = New-Object System.Drawing.SolidBrush($col)
    $g.FillEllipse($b, $ox,        $oy+10*$s, 34*$s, 24*$s)
    $g.FillEllipse($b, $ox+18*$s,  $oy,       30*$s, 30*$s)
    $g.FillEllipse($b, $ox+38*$s,  $oy+6*$s,  30*$s, 26*$s)
    $g.FillRectangle($b, $ox+14*$s, $oy+18*$s, 44*$s, 16*$s)
    $b.Dispose()
}
Cloud 92 50 1.0 ([System.Drawing.Color]::FromArgb(255,205,222,238))  # shaded underside
Cloud 92 46 1.0 ([System.Drawing.Color]::FromArgb(255,255,255,255))  # highlight on top

# --- bottom dark gradient for text legibility ------------------------------
$brect = New-Object System.Drawing.Rectangle(0,($H-52),$W,52)
$bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush($brect,
        [System.Drawing.Color]::FromArgb(0,8,18,33),
        [System.Drawing.Color]::FromArgb(190,8,18,33),90)
$g.FillRectangle($bg, $brect)

# --- title + subtitle -------------------------------------------------------
$title    = New-Object System.Drawing.Font("Segoe UI", 24, [System.Drawing.FontStyle]::Bold)
$subFont  = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Regular)
$shadow   = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(170,0,0,0))
$white    = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
$sub      = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(220,200,224,244))
$g.DrawString("Meteo3DS", $title, $shadow, 11, 75)
$g.DrawString("Meteo3DS", $title, $white,  9,  73)
$g.DrawString("Live weather | Open-Meteo", $subFont, $sub, 11, 108)

$g.Dispose()
$out = Join-Path $PSScriptRoot "banner.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "banner.png written: $out"
