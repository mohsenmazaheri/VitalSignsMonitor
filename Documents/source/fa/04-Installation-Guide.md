---
title: راهنمای نصب و راه‌اندازی
subtitle: VitalSigns Monitor - ابزارها، ساخت، اجرای اول، توزیع و درگاه‌های سریال مجازی
docid: VSM-INS-001
version: 1.0
date: ۱۰ آبان ۱۴۰۵
author: محسن مظاهری
status: پیش‌نویس برای بررسی
lang: fa
output: FA/VitalSignsMonitor-04-Installation-Guide-FA.docx
header: VitalSigns Monitor · راهنمای نصب و راه‌اندازی
history: 1.0 | ۱۴۰۵/۰۸/۱۰ | محسن مظاهری | انتشار اول، بر اساس نسخه 0.1.0 نرم‌افزار
notice: **فقط داده مصنوعی - این نرم‌افزار وسیله پزشکی نیست.** برنامه‌ها را فقط برای آموزش، نمایش و آزمون نرم‌افزار نصب کنید.
---

# ۱- مقدمه

این راهنما توضیح می‌دهد چگونه ابزارهای توسعه را نصب کنید، VitalSigns Monitor را از کد منبع بسازید، آن را برای اولین بار اجرا کنید و به یک رایانه ویندوزی دیگر منتقل کنید. درگاه‌های سریال مجازی برای آزمون بدون سخت‌افزار هم پوشش داده شده‌اند.

| بخش | مخاطب |
|---|---|
| ۲ تا ۸ | همه کسانی که برنامه‌ها را از کد منبع می‌سازند (ویندوز) |
| ۹ | انتقال برنامه‌های آماده به رایانه‌ای بدون Qt |
| ۱۰ | آزمون اتصال سریال بدون سخت‌افزار |
| ۱۱ | لینوکس |
| ۱۳ | مشکلات هنگام نصب |

پیکربندی بررسی‌شده: ویندوز ۱۱ Pro شصت‌وچهاربیتی، Qt 6.12.0 MinGW 64-bit، MinGW 13.1، CMake همراه نصب Qt، Ninja و Qt Creator. گام‌هایی که با «هنوز بررسی نشده» مشخص شده‌اند از روش استاندارد Qt پیروی می‌کنند ولی روی یک رایانه تمیز آزموده نشده‌اند.

# ۲- نیازمندی‌های سیستم

| مورد | حداقل / پیشنهادی |
|---|---|
| سیستم‌عامل | ویندوز ۱۰ یا ۱۱ شصت‌وچهاربیتی (روی ویندوز ۱۱ بررسی شده). لینوکس شصت‌وچهاربیتی: بخش ۱۱ |
| پردازنده | x86-64 با دست‌کم ۲ هسته (برای ساخت ۴ هسته یا بیشتر پیشنهاد می‌شود) |
| حافظه | برای ساخت ۸ گیگابایت پیشنهاد می‌شود؛ خود برنامه‌ها حافظه کمی لازم دارند |
| فضای دیسک | تا حدود ۱۵ گیگابایت برای کیت Qt 6.12.0 MinGW با همه بخش‌های اختیاری (۱۴ گیگابایت با فایل‌های اطلاعات اشکال‌زدایی اندازه‌گیری شد)، حدود ۰٫۷ گیگابایت برای MinGW و ۲۵۰ تا ۴۰۰ مگابایت برای هر پوشه ساخت Debug |
| نمایشگر | ۱۲۸۰ در ۸۰۰ یا بیشتر (پنل شبیه‌ساز در اندازه ۱۲۰۰ در ۷۷۰ طراحی شده و مقیاس می‌خورد) |
| گرافیک | راه‌انداز سازگار با OpenGL، Direct3D 11 یا Vulkan (مورد استفاده Qt Quick) |
| شبکه | دسترسی به اینترنت برای نصب‌کننده Qt و اولین اجرای CMake (دانلود GoogleTest) |

# ۳- نرم‌افزارهای پیش‌نیاز

| نرم‌افزار | نسخه | کاربرد | منبع |
|---|---|---|---|
| Qt (متن‌باز) | 6.8 یا جدیدتر؛ بررسی‌شده با 6.12.0 | چارچوب: Core، Gui، Network، Serial Port، Quick، Quick Controls 2 | Qt Online Installer (حساب رایگان Qt) |
| MinGW | 13.1 (64-bit) | کامپایلر C++20 (g++) | نصب‌کننده Qt، بخش Developer and Designer Tools |
| CMake | 3.25 یا جدیدتر | سامانه ساخت | نصب‌کننده Qt یا cmake.org |
| Ninja | هر نسخه جدید | ابزار ساخت سریع | نصب‌کننده Qt |
| Qt Creator | هر نسخه جدید | محیط توسعه (اختیاری ولی پیشنهادی) | نصب‌کننده Qt |
| Git | هر نسخه جدید | دریافت کد منبع | git-scm.com |
| GoogleTest | 1.15.2 | آزمون‌های واحد | خودکار توسط CMake دانلود می‌شود |
| com0com | اختیاری | جفت درگاه COM مجازی برای آزمون سریال | پروژه «com0com» در SourceForge |

جایگزین: Visual Studio 2022 با کیت Qt از نوع msvc2022_64 (preset به نام windows-msvc). روی این رایانه بررسی نشده است.

# ۴- نصب Qt و ابزارها

1. Qt Online Installer را از qt.io (نسخه متن‌باز) دانلود کنید و با یک حساب رایگان Qt وارد شوید.
2. «Custom installation» را انتخاب کنید و اجزای جدول زیر را علامت بزنید.
3. نصب را تمام کنید (پوشه پیش‌فرض C:\Qt).

| جزء (در درخت نصب‌کننده) | لازم است؟ |
|---|---|
| Qt > Qt 6.12.0 > MinGW 13.1.0 64-bit | بله |
| Qt > Qt 6.12.0 > Additional Libraries > Qt Serial Port | بله - بدون آن CMake با خطای «Qt6SerialPort not found» متوقف می‌شود |
| Qt > Developer and Designer Tools > MinGW 13.1.0 64-bit | بله |
| Qt > Developer and Designer Tools > CMake و Ninja | بله |
| Qt > Developer and Designer Tools > Qt Creator | پیشنهادی |
| Qt > Qt 6.12.0 > Sources و Qt Debug Information Files | اختیاری (برای اشکال‌زدایی درون Qt) |

> اگر Qt از قبل نصب است، بخش‌های جاافتاده را با `C:\Qt\MaintenanceTool.exe` و گزینه Add or remove components اضافه کنید.

پس از نصب، مهم‌ترین پوشه‌ها این‌ها هستند:

| پوشه | محتوا |
|---|---|
| C:\Qt\6.12.0\mingw_64 | کتابخانه‌های Qt و پوشه bin با DLLها و windeployqt.exe |
| C:\Qt\Tools\mingw1310_64\bin | g++، gdb و DLLهای زمان اجرای MinGW |
| C:\Qt\Tools\CMake_64\bin | cmake.exe و ctest.exe |
| C:\Qt\Tools\Ninja | ninja.exe |

# ۵- دریافت کد منبع

مخزن را clone کنید یا پوشه پروژه را کپی کنید:

```
git clone https://github.com/mohsenmazaheri/VitalSignsMonitor.git
```

مسیری را انتخاب کنید که اجازه نوشتن در آن دارید. مسیرهای دارای فاصله مشکلی ندارند (پروژه در «D:\Projects\Vital Signs Monitor» توسعه داده می‌شود).

# ۶- ساخت با Qt Creator

1. Qt Creator را اجرا کنید، از منوی File گزینه Open File or Project را بزنید و فایل `CMakeLists.txt` سطح بالا را انتخاب کنید.
2. در صفحه «Configure Project» کیت **Desktop Qt 6.12.0 MinGW 64-bit** را انتخاب و روی **Configure Project** کلیک کنید. ممکن است Qt Creator کیت‌هایی از CMakePresets.json هم پیشنهاد کند (mingw-debug و windows-msvc)؛ آن‌ها لازم نیستند.
3. صبر کنید تا CMake تمام شود. اولین اجرا GoogleTest را دانلود می‌کند.
4. با **Build > Build All Projects** (Ctrl+Shift+B) بسازید.
5. در انتخاب‌گر کیت پایین سمت چپ (ستون Run)، اول **VitalSignsPatientSimulator** را انتخاب و با Ctrl+R اجرا کنید.
6. سپس **VitalSignsMonitor** را انتخاب و آن را هم اجرا کنید.

> **تنظیم مهم:** Qt Creator به‌طور پیش‌فرض پیش از ساخت، همه برنامه‌های در حال اجرای همان پروژه را می‌بندد. در این حالت با اجرای مانیتور، شبیه‌ساز بسته می‌شود. گزینه Edit > Preferences > Build & Run > General > **Stop applications before building** را روی **Same Application** بگذارید. راه دیگر: برنامه دوم را با Build > Run Without Deployment اجرا کنید.

فایل‌های اجرایی در `build\Desktop_Qt_6_12_0_MinGW_64_bit_Debug\bin` ساخته می‌شوند.

# ۷- ساخت از خط فرمان

در Git Bash:

```
export QTDIR=C:/Qt/6.12.0/mingw_64
export PATH="$QTDIR/bin:/c/Qt/Tools/mingw1310_64/bin:/c/Qt/Tools/Ninja:/c/Qt/Tools/CMake_64/bin:$PATH"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

در PowerShell:

```
$env:QTDIR = "C:/Qt/6.12.0/mingw_64"
$env:PATH = "C:\Qt\6.12.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;$env:PATH"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
```

برنامه‌ها در `build\mingw-debug\bin` قرار می‌گیرند. بدون preset:

```
cmake -S . -B build/mingw-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/mingw-debug --parallel
```

گزینه‌های ساخت: `-DVSM_BUILD_TESTS=OFF` آزمون‌ها (و دانلود GoogleTest) را حذف می‌کند، `-DVSM_BUILD_SIMULATOR=OFF` شبیه‌سازها را حذف می‌کند و `-DVSM_WARNINGS_AS_ERRORS=ON` هشدارها را به خطا تبدیل می‌کند.

# ۸- بررسی نصب

## ۸-۱ آزمون‌های واحد

```
ctest --preset mingw-debug
```

نتیجه مورد انتظار: «100% tests passed, 0 tests failed out of 63». در Qt Creator: Tools > Tests > Run All Tests.

## ۸-۲ اجرای اول

1. فایل `VitalSignsPatientSimulator.exe` را اجرا کنید. پنل جلو ظاهر می‌شود، LED به نام POWER روشن است و LCD یک ECG نشان می‌دهد.
2. فایل `VitalSignsMonitor.exe` را اجرا و روی **Connect** کلیک کنید (TCP، 127.0.0.1، 5555).
3. موج ECG حرکت می‌کند، HR و SpO2 ظاهر می‌شوند و LED به نام LINK در شبیه‌ساز روشن می‌شود.

> بیرون از Qt Creator باید پوشه `C:\Qt\6.12.0\mingw_64\bin` در PATH باشد، وگرنه ویندوز خطای پیدا نشدن Qt6Core.dll را نشان می‌دهد. برای رایانه‌های دیگر بخش ۹ را ببینید.

میان‌بر خط فرمان: `VitalSignsMonitor --connect 127.0.0.1:5555`.

# ۹- توزیع روی یک رایانه ویندوزی دیگر

هنوز روی یک رایانه تمیز بررسی نشده است. این گام‌ها روش استاندارد توزیع Qt با windeployqt را دنبال می‌کنند.

1. یک نسخه Release بسازید:

```
cmake -S . -B build/mingw-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DVSM_BUILD_TESTS=OFF
cmake --build build/mingw-release --parallel
```

2. یک پوشه بسازید و فایل‌های اجرایی را در آن کپی کنید:

```
mkdir deploy
copy build\mingw-release\bin\VitalSignsMonitor.exe deploy\
copy build\mingw-release\bin\VitalSignsPatientSimulator.exe deploy\
copy build\mingw-release\bin\VitalSignsSimulator.exe deploy\
```

3. با windeployqt کتابخانه‌ها و ماژول‌های QML کیوت را کپی کنید (از خط فرمانی که پوشه‌های bin کیوت و MinGW در PATH آن هستند):

```
windeployqt --release --qmldir src\app\qml deploy\VitalSignsMonitor.exe
windeployqt --release --qmldir tools\simulator\gui\qml deploy\VitalSignsPatientSimulator.exe
windeployqt --release deploy\VitalSignsSimulator.exe
```

4. بررسی کنید DLLهای زمان اجرای MinGW در پوشه باشند (libgcc_s_seh-1.dll، libstdc++-6.dll و libwinpthread-1.dll). اگر نیستند، آن‌ها را از `C:\Qt\Tools\mingw1310_64\bin` کپی کنید.
5. کل پوشه `deploy` را به رایانه دیگر کپی و برنامه‌ها را در آن‌جا اجرا کنید. پیش از تحویل پوشه، هر دو برنامه را آزمایش کنید.

کتابخانه‌های متن‌باز Qt تحت مجوز LGPL v3 هستند: هنگام تحویل پوشه به دیگران، شرایط مجوز Qt (پیوند پویا، متن مجوزها، دسترسی به کد منبع Qt) رعایت شود.

# ۱۰- درگاه‌های سریال مجازی (com0com)

مسیر سریال به دو درگاه سریال نیاز دارد که به هم وصل باشند. بدون سخت‌افزار، یک جفت مجازی این کار را انجام می‌دهد.

1. com0com را از صفحه پروژه آن در SourceForge دانلود کنید. از بسته امضاشده ۶۴ بیتی استفاده کنید.
2. نصب‌کننده را با دسترسی مدیر (administrator) اجرا کنید. یک جفت مانند CNCA0 و CNCB0 ساخته می‌شود.
3. از گروه com0com در منوی شروع، «Setup Command Prompt» را باز کنید و نام‌های معمولی به درگاه‌ها بدهید:

```
change CNCA0 PortName=COM5
change CNCB0 PortName=COM6
```

4. در شبیه‌ساز SERIAL، درگاه COM5، باود 115200 و OPEN را انتخاب کنید. در مانیتور Serial، درگاه COM6، باود 115200 و Connect را بزنید.

> ویندوز ۱۱ با Secure Boot ممکن است راه‌اندازهای قدیمی بدون امضا را نپذیرد. اگر راه‌انداز نصب نشد، از دو مبدل USB به سریال که با کابل null-modem به هم وصل شده‌اند یا از ابزار دیگری برای درگاه سریال مجازی استفاده کنید. مسیر سریال نسخه 0.1.0 هنوز روی این رایانه به‌صورت سرتاسری آزموده نشده است.

# ۱۱- لینوکس

هنوز بررسی نشده؛ ساخت لینوکس برای مرحله M3 (در WSL2) برنامه‌ریزی شده است.

1. کیت رومیزی Qt 6.8 یا جدیدتر (gcc_64) را با Qt Online Installer و همراه با Qt Serial Port نصب کنید. بسته‌های توزیع‌ها اغلب قدیمی‌تر از Qt 6.8 هستند.
2. ابزارهای ساخت را نصب کنید، مثلاً در Debian یا Ubuntu: `sudo apt install build-essential ninja-build libgl1-mesa-dev` و یک CMake با نسخه 3.25 یا جدیدتر.
3. بسازید:

```
export QTDIR=~/Qt/6.12.0/gcc_64
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

4. درگاه‌های سریال مانند /dev/ttyUSB0 به گروه dialout نیاز دارند: دستور `sudo usermod -aG dialout $USER` را اجرا و دوباره وارد حساب شوید. جفت‌های مجازی را می‌توان با socat ساخت.

# ۱۲- شبکه و دیوار آتش

- سرور TCP شبیه‌ساز فقط روی 127.0.0.1 گوش می‌دهد، پس رایانه‌های دیگر نمی‌توانند به آن وصل شوند.
- مانیتور فقط اتصال خروجی برقرار می‌کند.
- اگر دیوار آتش ویندوز درباره شبیه‌ساز پرسید، اجازه برای شبکه‌های خصوصی کافی است؛ اتصال‌های محلی در هر حال کار می‌کنند.
- درگاه پیش‌فرض 5555 است. اگر برنامه دیگری از آن استفاده می‌کند، شبیه‌ساز را با `--port 5556` اجرا کنید و همان درگاه را در مانیتور وارد کنید.

# ۱۳- رفع مشکل

| پیام یا نشانه | علت | راه حل |
|---|---|---|
| CMake: Failed to find required Qt component "SerialPort" | ماژول Qt Serial Port برای کیت انتخاب‌شده نصب نیست | MaintenanceTool > Add or remove components > Qt 6.12.0 > Additional Libraries > Qt Serial Port، سپس دوباره CMake را اجرا کنید |
| CMake: Could not find a package configuration file provided by "Qt6" | CMAKE_PREFIX_PATH یا QTDIR تنظیم نشده | QTDIR را تنظیم کنید (برای presetها) یا -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 را بدهید |
| CMake نمی‌تواند GoogleTest را دانلود کند | در اولین پیکربندی اینترنت وصل نیست | یک بار به اینترنت وصل شوید یا با -DVSM_BUILD_TESTS=OFF پیکربندی کنید |
| هنگام اجرای فایل exe پیام «Qt6Core.dll was not found» | پوشه bin کیوت در PATH نیست | از Qt Creator اجرا کنید، C:\Qt\6.12.0\mingw_64\bin را به PATH اضافه کنید یا توزیع کنید (بخش ۹) |
| هنگام شروع یک ماژول QML پیدا نمی‌شود | توزیع ناقص | windeployqt را با گزینه --qmldir مطابق بخش ۹ اجرا کنید |
| با اجرای مانیتور از Qt Creator، شبیه‌ساز بسته می‌شود | Qt Creator پیش از ساخت، برنامه‌های همان پروژه را می‌بندد | گزینه «Stop applications before building» را روی «Same Application» بگذارید (بخش ۶) |
| درگاه TCP در شبیه‌ساز باز نمی‌شود | درگاه در حال استفاده است، مثلاً توسط شبیه‌ساز دوم | برنامه دیگر را ببندید یا از --port 5556 استفاده کنید |
| فهرست درگاه‌های سریال خالی است | هیچ درگاه سریال واقعی یا مجازی وجود ندارد | com0com را نصب کنید (بخش ۱۰) یا یک مبدل وصل کنید و با ↻ فهرست را تازه کنید |
| کیت «windows-msvc» کار نمی‌کند | Visual Studio 2022 یا کیت MSVC کیوت نصب نیست | از کیت MinGW استفاده کنید، یا Visual Studio 2022 و کیت msvc2022_64 کیوت را نصب کنید |

# ۱۴- حذف نصب

- پوشه پروژه (و در صورت وجود، پوشه deploy) را پاک کنید. نسخه 0.1.0 هیچ تنظیماتی از خودش ذخیره نمی‌کند (نه QSettings و نه رجیستری).
- Qt را با `C:\Qt\MaintenanceTool.exe` و گزینه Remove all components حذف کنید.
- com0com را با حذف‌کننده آن از فهرست برنامه‌های ویندوز حذف کنید.
