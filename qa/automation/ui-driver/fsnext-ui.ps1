# FsNext UI-driver — thư viện lái UI cho test end-to-end / scenario / monkey.
# Dùng: .  qa\automation\ui-driver\fsnext-ui.ps1   rồi gọi các hàm Fs-*.
# Lưu ý: native file/folder picker + màn hình khóa (LogonUI) chặn automation —
# xem qa/README §1. Tắt auto-lock/sleep khi chạy.

Add-Type -AssemblyName System.Windows.Forms,System.Drawing
Add-Type @'
using System; using System.Runtime.InteropServices;
public class FsUI {
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, IntPtr pid);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool f);
  [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, IntPtr e);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public static void Front(IntPtr h){ uint fg=GetWindowThreadProcessId(GetForegroundWindow(),IntPtr.Zero); uint me=GetCurrentThreadId(); AttachThreadInput(fg,me,true); ShowWindow(h,3); BringWindowToTop(h); SetForegroundWindow(h); AttachThreadInput(fg,me,false); }
  public static void Click(int x,int y){ SetCursorPos(x,y); System.Threading.Thread.Sleep(130); mouse_event(0x02,0,0,0,IntPtr.Zero); System.Threading.Thread.Sleep(70); mouse_event(0x04,0,0,0,IntPtr.Zero); }
}
'@ -ReferencedAssemblies System.Drawing

$global:FsExe = "D:\Work\FsNext\output\FsNext.exe"

function Fs-Locked { [bool](Get-Process LogonUI -ErrorAction SilentlyContinue) }
function Fs-Alive  { [bool](Get-Process FsNext  -ErrorAction SilentlyContinue) }
function Fs-Hwnd   { (Get-Process FsNext -ErrorAction SilentlyContinue | Where-Object {$_.MainWindowHandle -ne 0} | Select-Object -First 1).MainWindowHandle }

# clean kill + launch + chờ cửa sổ ổn định (tránh race single-instance)
function Fs-Launch {
  Get-Process FsNext -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
  $n=0; while ((Get-Process FsNext -ErrorAction SilentlyContinue) -and $n -lt 20){ Start-Sleep -Milliseconds 400; $n++ }
  Start-Sleep -Seconds 2
  Start-Process -FilePath $global:FsExe
  $p=$null; $n=0
  while(-not $p -and $n -lt 30){ Start-Sleep -Milliseconds 700; $p=Get-Process FsNext -ErrorAction SilentlyContinue | Where-Object {$_.MainWindowHandle -ne 0} | Select-Object -First 1; $n++ }
  return [bool]$p
}

# đưa lên trước + maximize, cache rect vào $global:FsRect (luôn gọi trước khi click)
function Fs-Front {
  $h = Fs-Hwnd; if (-not $h) { return $null }
  [FsUI]::Front($h); Start-Sleep -Milliseconds 650
  $r = New-Object FsUI+RECT; [FsUI]::GetWindowRect($h,[ref]$r) | Out-Null
  $global:FsRect = $r; return $h
}

# click toạ độ TƯƠNG ĐỐI cửa sổ (warm=true: click trung tính trước để hút cú nuốt-input đầu)
function Fs-Click($x,$y,$warm=$true) {
  if ($warm) { [FsUI]::SetCursorPos($global:FsRect.L + 1088, $global:FsRect.T + 357); Start-Sleep -Milliseconds 200 }
  [FsUI]::Click($global:FsRect.L + $x, $global:FsRect.T + $y)
}
# click toạ độ MÀN HÌNH tuyệt đối (cho native dialog — dùng full-screen capture để định vị)
function Fs-ClickScreen($x,$y) { [FsUI]::Click($x,$y) }
function Fs-Type($s) { [System.Windows.Forms.SendKeys]::SendWait($s) }

# chụp cửa sổ FsNext (PrintWindow — bắt được cả khi bị che; KHÔNG bắt native dialog)
function Fs-Grab($path) {
  $h = Fs-Hwnd; $r = $global:FsRect; $w = $r.R-$r.L; $ht = $r.B-$r.T
  $bmp = New-Object System.Drawing.Bitmap $w,$ht
  $g = [System.Drawing.Graphics]::FromImage($bmp); $hdc = $g.GetHdc()
  [FsUI]::PrintWindow($h,$hdc,2) | Out-Null; $g.ReleaseHdc($hdc)
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
}
# chụp toàn màn hình (cho native file/folder picker — toạ độ 1:1 nếu DPI 100%)
function Fs-GrabScreen($path) {
  $b=[System.Windows.Forms.Screen]::PrimaryScreen.Bounds
  $bmp=New-Object System.Drawing.Bitmap $b.Width,$b.Height
  $g=[System.Drawing.Graphics]::FromImage($bmp); $g.CopyFromScreen($b.Location,[System.Drawing.Point]::Empty,$b.Size)
  $bmp.Save($path,[System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
}

# kiểm crash trong N phút gần đây (heap=0xc0000374, AV=0xc0000005)
function Fs-CrashWatch($minutes=10) {
  $since=(Get-Date).AddMinutes(-$minutes)
  Get-WinEvent -FilterHashtable @{LogName='Application'; Level=2; StartTime=$since} -ErrorAction SilentlyContinue |
    Where-Object { $_.Message -match 'FsNext' } |
    ForEach-Object { ($_.Message -split "`n")[0..5] -join ' | ' }
}

# login bằng account test (cửa sổ phải maximize — verify rect trước)
function Fs-Login($email, $pass) {
  Fs-Front | Out-Null
  Fs-Click 1230 556 $true;  Start-Sleep -Milliseconds 250
  Fs-Type "^a"; Fs-Type "{DEL}"; Fs-Type $email; Start-Sleep -Milliseconds 250
  Fs-Click 1230 628 $false; Start-Sleep -Milliseconds 250
  Fs-Type "^a"; Fs-Type "{DEL}"; Fs-Type $pass;  Start-Sleep -Milliseconds 250
  Fs-Click 1240 722 $false; Start-Sleep -Seconds 5
}
