---
title: راهنمای توسعه‌دهنده
subtitle: VitalSigns Monitor - معماری، ساختار کد، ساخت، آزمون و توسعه
docid: VSM-DEV-001
version: 1.2
date: ۱۱ آبان ۱۴۰۵
author: محسن مظاهری
status: پیش‌نویس برای بررسی
lang: fa
output: FA/VitalSignsMonitor-02-Developer-Guide-FA.docx
header: VitalSigns Monitor · راهنمای توسعه‌دهنده
history: 1.0 | ۱۴۰۵/۰۸/۱۰ | محسن مظاهری | انتشار اول، بر اساس نسخه 0.1.0 نرم‌افزار; 1.1 | ۱۴۰۵/۰۸/۱۰ | محسن مظاهری | مدیر هشدار، ثبت رویداد، صدای هشدار و CI (نسخه 0.2.0 نرم‌افزار); 1.2 | ۱۴۰۵/۰۸/۱۱ | محسن مظاهری | زمان‌سنج نگهبان دقیق‌تر، مقادیر ناممکن، تحلیل ریسک و ردیابی‌پذیری (نسخه 0.3.0 نرم‌افزار)
notice: **فقط داده مصنوعی - این نرم‌افزار وسیله پزشکی نیست.** هرگز داده واقعی بیمار را به کد، آزمون‌ها یا مخزن اضافه نکنید.
---

# ۱- مقدمه

## ۱-۱ هدف و مخاطبان

این راهنما برای توسعه‌دهندگانی است که VitalSigns Monitor را می‌سازند، تغییر می‌دهند یا بازبینی می‌کنند. در آن معماری، نقش هر ماژول، فرایند ساخت و آزمون و گام‌های رایج‌ترین توسعه‌ها توضیح داده شده است. نصب ابزارها در راهنمای نصب و نحوه کار با برنامه‌ها در راهنمای کاربری آمده است.

## ۱-۲ اسناد مرتبط

| سند | محتوا |
|---|---|
| README.md | نمای کلی، قابلیت‌ها، ساخت و اجرای سریع |
| docs/ARCHITECTURE.md | لایه‌ها، رشته‌ها، نگاشت SOLID، مدل هشدار |
| docs/PROTOCOL.md | شرح بایت به بایت پروتکل دستگاه |
| LEARNING.md | دفترچه یادگیری با نکات طراحی و پرسش‌های مصاحبه |
| CLAUDE.md | قواعد کوتاه برای کار با دستیار هوش مصنوعی روی کد |
| VSM-RFP-001 | نیازمندی‌ها با شناسه (FR-... و NFR-...) |
| docs/medical/ | نیازمندی‌های نرم‌افزار، تحلیل ریسک و ردیابی‌پذیری، به‌عنوان تمرین آموزشی |

# ۲- فناوری‌ها

| حوزه | انتخاب | توضیح |
|---|---|---|
| زبان | C++20 | std::span، std::variant، std::optional، designated initializers، عملگر == پیش‌فرض |
| چارچوب | Qt نسخه 6.8 یا جدیدتر | ساخته و آزموده شده با Qt 6.12.0 (MinGW 64-bit) |
| رابط کاربری | Qt Quick / QML و Qt Quick Controls (سبک Fusion) | رسم ECG با گراف صحنه Qt Quick |
| ورودی/خروجی | Qt Network (QTcpSocket، QTcpServer) و Qt Serial Port | |
| صدا | Qt Multimedia (QSoundEffect) | ملودی‌ها در کد ساخته می‌شوند |
| ساخت | CMake 3.25+ و Ninja یا MinGW Makefiles | فایل CMakePresets.json موجود است |
| آزمون | GoogleTest 1.15 (هنگام پیکربندی دانلود می‌شود) و CTest | ۱۴۰ آزمون، همراه با بررسی ردیابی‌پذیری |
| کامپایلرها | GCC 13 (MinGW)، MSVC 2022، Clang 14+ | هشدارها فعال؛ بدون هیچ هشدار |

# ۳- ساختار مخزن

```
VitalSignsMonitor/
  CMakeLists.txt          top-level project, options, Qt lookup
  CMakePresets.json       mingw-debug, windows-msvc, linux-debug
  cmake/                  CompilerWarnings.cmake (warning policy)
  src/
    protocol/             wire format: Frame, FrameDecoder, Messages, CRC-16   (C++20, no Qt)
    core/                 domain: VitalSigns, alarms, SweepBuffer             (C++20, no Qt)
    audit/                FileAuditLog: audit trail as a CSV file             (C++20, no Qt)
    device/               ITransport, TCP/serial transports, DeviceSession, DeviceController
    ui/                   shared QML module VitalSigns.Ui: WaveformSeries, WaveformItem
    app/                  monitor: main.cpp, view models, QML
  tools/simulator/
    engine/               vsm_simulation library: patient model, scenarios, sinks
    cli/                  VitalSignsSimulator (console)
    gui/                  VitalSignsPatientSimulator (front panel, QML)
  tests/                  GoogleTest suites (protocol, core, device, simulation)
  docs/                   ARCHITECTURE.md, PROTOCOL.md, medical/, images/
  tools/check-traceability.ps1   checks docs/medical against code and tests
  .github/workflows/      ci.yml: build and test on Windows and Linux
  Documents/              this document set (Word) and its sources
```

# ۴- معماری

## ۴-۱ لایه‌ها

| لایه | پوشه | وابسته به | نوع‌های اصلی |
|---|---|---|---|
| app | src/app | ui، device، audit، core، Qt Quick، Qt Multimedia | main.cpp، MonitorViewModel، AlarmListModel، AuditLogModel، QtAlarmSound، QML |
| audit | src/audit | core و کتابخانه استاندارد C++ | FileAuditLog |
| ui | src/ui | core، Qt Quick | WaveformSeries، WaveformItem |
| device | src/device | protocol، core، Qt Network، Qt Serial Port | ITransport، TcpTransport، SerialTransport، DeviceSession، DeviceController |
| core | src/core | فقط کتابخانه استاندارد C++ | VitalSigns، Alarm، AlarmLimits، IAlarmRule، AlarmEvaluator، AlarmManager، AlarmTone، IAuditLog، SweepBuffer |
| protocol | src/protocol | فقط کتابخانه استاندارد C++ | Frame، FrameDecoder، Messages، crc16CcittFalse |
| simulation | tools/simulator/engine | protocol، Qt Network، Qt Serial Port | SyntheticPatient، Scenario، DeviceSimulator، IByteSink و خروجی‌ها |
| برنامه‌های شبیه‌ساز | tools/simulator/cli و gui | simulation (و ui برای نسخه گرافیکی) | main.cpp خط فرمان، PatientSimulatorController، QML پنل جلو |

قواعد وابستگی:

- لایه‌های `protocol` و `core` بدون Qt می‌مانند تا در چند میلی‌ثانیه کامپایل و آزموده شوند و بتوانند به سکوهای دیگر منتقل شوند.
- لایه `core` هرگز `protocol` را include نمی‌کند. لایه device پیام‌های سیمی (NumericsMessage) را به اشیای دامنه (VitalSigns) تبدیل می‌کند، پس قالب سیمی می‌تواند بدون دست زدن به منطق هشدار تغییر کند.
- کلاس‌های واقعی فقط در فایل‌های `main.cpp` (ریشه ترکیب) ساخته می‌شوند. بقیه کد فقط رابط‌ها یا کارخانه‌ها را دریافت می‌کند.

## ۴-۲ هدف‌های CMake

| هدف | نوع | فضای نام | محتوا |
|---|---|---|---|
| vsm_protocol (vsm::protocol) | کتابخانه ایستا | vsm::protocol | لایه پروتکل |
| vsm_core (vsm::core) | کتابخانه ایستا | vsm::core | دامنه و هشدارها |
| vsm_audit (vsm::audit) | کتابخانه ایستا | vsm::audit | فایل ثبت رویداد |
| vsm_device (vsm::device) | کتابخانه ایستا | vsm::device | ترابرها، نشست، کنترل‌گر |
| vsm_ui (vsm::ui) + vsm_uiplugin | ماژول QML ایستا به نام VitalSigns.Ui | vsm::ui | نوع‌های رسم موج |
| vsm_simulation (vsm::simulation) | کتابخانه ایستا | vsm::simulator | موتور شبیه‌ساز |
| VitalSignsMonitor | فایل اجرایی، ماژول QML به نام VitalSigns.App | vsm::app | مانیتور |
| VitalSignsPatientSimulator | فایل اجرایی، ماژول QML به نام VitalSigns.PatientSimulator | vsm::simulator | شبیه‌ساز با پنل جلو |
| VitalSignsSimulator | فایل اجرایی خط فرمان | - | شبیه‌ساز خط فرمان |
| vsm_unit_tests، vsm_device_tests، vsm_simulation_tests، vsm_app_tests | فایل‌های اجرایی آزمون | - | مجموعه‌های GoogleTest |

## ۴-۳ رشته‌ها و جریان داده

```
I/O thread                                             GUI thread
----------                                             ----------
QTcpSocket / QSerialPort
  -> ITransport::dataReceived(bytes)
  -> DeviceSession: FrameDecoder -> parseMessage
     -> vitalsReceived(VitalSigns)        ==queued==>  DeviceController (relay)
     -> ecgSamplesReceived(mV, rate)      ==queued==>    -> MonitorViewModel
     -> linkLostChanged / statistics      ==queued==>         -> AlarmEvaluator -> AlarmManager -> AlarmListModel, IAlarmSound, IAuditLog
                                                              -> WaveformSeries -> WaveformItem
                                                              -> QML properties
```

- `DeviceController` در رشته رابط کاربری زندگی می‌کند و صاحب یک QThread است. برای هر اتصال یک DeviceSession می‌سازد، آن را به رشته ورودی/خروجی منتقل می‌کند و با یک فراخوانی صف‌شده (queued) شروع می‌کند.
- ترابر فرزند نشست است، پس سوکت هم همراه آن به رشته ورودی/خروجی منتقل می‌شود.
- هنگام قطع اتصال، نشست داخل رشته ورودی/خروجی متوقف می‌شود (سوکت‌ها و زمان‌سنج‌ها باید در رشته خودشان استفاده شوند)، سپس برگردانده و در رشته رابط کاربری حذف می‌شود.
- رشته‌ها فقط نوع‌های مقداری (VitalSigns، فهرست float، LinkStatistics) را از طریق سیگنال‌های صف‌شده رد و بدل می‌کنند. هیچ حالت مشترک تغییرپذیر و هیچ mutex وجود ندارد.
- تابع `WaveformItem::updatePaintNode()` در رشته رندر و در زمانی اجرا می‌شود که رشته رابط کاربری مسدود است، پس می‌تواند بدون قفل WaveformSeries را بخواند.

## ۴-۴ اصول SOLID و الگوهای طراحی

| اصل یا الگو | محل به‌کارگیری |
|---|---|
| مسئولیت یگانه | FrameDecoder فقط فریم‌ها را جدا می‌کند؛ parseMessage فقط داده را رمزگشایی می‌کند؛ DeviceSession یک ارتباط را اداره می‌کند؛ AlarmEvaluator فقط ارزیابی و رتبه‌بندی می‌کند؛ WaveformItem فقط رسم می‌کند |
| باز/بسته | هشدار جدید = IAlarmRule جدید؛ ترابر جدید = ITransport جدید؛ خروجی جدید شبیه‌ساز = IByteSink جدید؛ سناریوی جدید = مدخل جدید در makeScenario() |
| جایگزینی لیسکوف | FakeTransport، TcpTransport و SerialTransport برای DeviceSession قابل جایگزینی‌اند (آزمون‌ها این را ثابت می‌کنند) |
| تفکیک رابط | ITransport چهار متد و سه سیگنال دارد؛ IByteSink از ITransport جداست |
| وارونگی وابستگی | DeviceSession به ITransport وابسته است؛ DeviceController یک TransportFactory می‌گیرد؛ AlarmEvaluator، AlarmManager و IAlarmSound به MonitorViewModel تزریق می‌شوند؛ AlarmManager در IAuditLog می‌نویسد؛ DeviceSimulator یک IByteSink و یک ساعت دریافت می‌کند |
| راهبرد (Strategy) | IAlarmRule با LimitAlarmRule و ConditionAlarmRule |
| نما (Facade) | DeviceController رشته و نشست را از رابط کاربری پنهان می‌کند |
| تزئین‌کننده (Decorator) | AuditLogModel هر رویداد را روی صفحه نشان می‌دهد و به IAuditLog واقعی می‌فرستد |
| ماشین حالت | AlarmManager: Active، Acknowledged، Latched |
| کارخانه (Factory) | createDefaultTransport()، createDefaultSink()، makeDefaultAlarmRules()، makeScenario() |
| MVVM | نماهای QML به MonitorViewModel، AlarmListModel، WaveformSeries و PatientSimulatorController متصل‌اند |
| ریشه ترکیب | src/app/main.cpp و tools/simulator/gui/main.cpp گراف اشیا را می‌سازند |

# ۵- ماژول‌ها

## ۵-۱ پروتکل (vsm::protocol)

- `Frame` و `encodeFrame()` یک فریم می‌سازند: همگام‌سازی `0xAA 0x55`، نسخه ۱، نوع پیام، شماره ترتیب، طول داده و CRC.
- `FrameDecoder::feed(bytes)` هر تکه‌ای از جریان بایت را می‌پذیرد و فریم‌های کامل و معتبر را برمی‌گرداند. آمار DecoderStatistics را نگه می‌دارد (فریم‌های رمزگشایی‌شده، خطاهای CRC، سرآیندهای نامعتبر، بایت‌های دورریخته).
- `parseMessage(frame)` یک `Message` (یک std::variant از HeartbeatMessage، NumericsMessage و EcgWaveformMessage) برمی‌گرداند، یا برای داده ناشناخته یا معیوب std::nullopt. تابع `toFrame()` عکس این کار را انجام می‌دهد.
- `crc16CcittFalse()` مقدار CRC-16/CCITT-FALSE را حساب می‌کند (چندجمله‌ای `0x1021`، مقدار اولیه `0xFFFF`؛ مقدار کنترلی رشته `"123456789"` برابر `0x29B1` است).

## ۵-۲ هسته (vsm::core)

- `VitalSigns` ضربان قلب و SpO2 اختیاری (optional) و دو پرچم حسگر را نگه می‌دارد. optional خالی یعنی «در دسترس نیست» و رابط کاربری «---» نشان می‌دهد.
- `AlarmContext` برابر است با VitalSigns به‌علاوه deviceLinkLost.
- `Alarm` یک کد پایدار (مثلاً HR_HIGH)، منبع، دسته، اولویت و متن پیام دارد.
- `IAlarmRule::evaluate(context)` یک هشدار یا std::nullopt برمی‌گرداند. `LimitAlarmRule` یک مقدار را با یک حد مقایسه می‌کند و `ConditionAlarmRule` از یک گزاره (predicate) استفاده می‌کند.
- `makeDefaultAlarmRules(AlarmLimits)` نُه قاعده پیش‌فرض را می‌سازد و حدود ناسازگار را رد می‌کند.
- `AlarmEvaluator` همه قواعد را ارزیابی می‌کند و از هر منبع فقط بالاترین اولویت را، مرتب‌شده بر اساس اولویت، نگه می‌دارد.
- `AlarmManager` به ارزیاب بدون حالت، حافظه اضافه می‌کند. هر هشدار در یکی از حالت‌های Active، Acknowledged یا Latched است. هشدارهای با اولویت بالا تا زمان تأیید می‌مانند؛ توقف صدا ۲ دقیقه طول می‌کشد و هشدار جدید با اولویت متوسط یا بالا آن را پایان می‌دهد. هر فراخوانی وابسته به زمان، «اکنون» را به‌صورت پارامتر می‌گیرد، پس آزمون‌ها هرگز منتظر نمی‌مانند.
- `IAuditLog` و `AuditEvent` ثبت رویداد را تعریف می‌کنند. مدیر هشدار فقط این رابط را می‌شناسد و هرگز با فایل کار نمی‌کند.
- `makeAlarmTone()` برای هر اولویت یک تکرار از ملودی را می‌سازد (بالا: ۱۰ پالس هر ۶ ثانیه، متوسط: ۳ پالس هر ۸ ثانیه، پایین: ۲ پالس هر ۲۰ ثانیه) و `encodeWav()` آن را به قالب فایل WAV درمی‌آورد.
- `SweepBuffer` بافر حلقوی پشت نمایش جاروبی ECG است.

زنجیره هشدار: AlarmEvaluator می‌گوید الان چه چیزی درست است، AlarmManager به یاد می‌سپارد اپراتور چه چیزی را دیده است، و مدل نما نتیجه را به فهرست هشدار و صدا می‌دهد.

## ۵-۳ دستگاه (vsm::device)

رابط ترابر:

```
class ITransport : public QObject {
    Q_OBJECT
public:
    enum class State { Disconnected, Connecting, Connected };
    virtual void open() = 0;
    virtual void close() = 0;
    virtual State state() const = 0;
    virtual QString description() const = 0;   // "tcp://127.0.0.1:5555", "serial://COM5@115200"
signals:
    void dataReceived(const QByteArray& data);
    void stateChanged(vsm::device::ITransport::State state);
    void errorOccurred(const QString& message);
};
```

- `TcpTransport` و `SerialTransport` این رابط را پیاده می‌کنند و `createDefaultTransport(ConnectionSettings)` یکی از آن‌ها را انتخاب می‌کند.
- `DeviceSession` ترابر را به FrameDecoder وصل می‌کند، پیام‌ها را به اشیای دامنه تبدیل می‌کند، برای داده عددی و ECG هر کدام یک زمان‌سنج نگهبان (`kDefaultWatchdogTimeout` برابر ۳۰۰۰ میلی‌ثانیه؛ ضربان حیات حساب نمی‌شود)، مقادیر ناممکن را نامعلوم حساب می‌کند و زمان‌سنج اتصال مجدد (`kDefaultReconnectDelay` برابر ۲۰۰۰ میلی‌ثانیه) را اجرا می‌کند و آمار LinkStatistics را می‌شمارد (فریم‌ها، خطاهای CRC، فریم‌های گم‌شده، پرش‌های شماره ترتیب، بایت‌های دورریخته).
- `DeviceController` صاحب رشته ورودی/خروجی است و نمایی (facade) است که مدل نما با آن صحبت می‌کند.
- دسته‌های لاگ: `vsm.device.controller`، `vsm.device.session`، `vsm.device.tcp` و `vsm.device.serial`؛ در برنامه `vsm.app.sound`.

## ۵-۴ رابط مشترک (vsm::ui، ماژول QML به نام VitalSigns.Ui)

- `WaveformSeries` نمونه‌هایی را که باید رسم شوند نگه می‌دارد (در C++ ساخته و به QML داده می‌شود).
- `WaveformItem` یک QQuickItem است که موج را با گراف صحنه رسم می‌کند (QSGGeometry در حالت DrawLines؛ فاصله جلوی جاروب رسم نمی‌شود).
- این ماژول یک کتابخانه ایستا است. هر برنامه‌ای که از آن استفاده می‌کند `vsm_uiplugin` را لینک می‌کند و `Q_IMPORT_QML_PLUGIN(VitalSigns_UiPlugin)` را فراخوانی می‌کند؛ ماژول QML خود برنامه هم `DEPENDENCIES VitalSigns.Ui` را اعلام می‌کند.

## ۵-۵ برنامه مانیتور (app)

- `main.cpp` گزینه `--connect host:port` را می‌خواند، DeviceController، AlarmEvaluator و MonitorViewModel را می‌سازد و ماژول QML به نام VitalSigns.App را بار می‌کند.
- `MonitorViewModel` ضربان قلب، SpO2، پرچم‌های حسگر، اولویت هشدارها، وضعیت اتصال، آمار ارتباط و آخرین خطا را در اختیار QML می‌گذارد و توابع connectTcp()، connectSerial()، disconnectDevice() و availableSerialPorts() را ارائه می‌کند.
- `AlarmListModel` مدل فهرستی پشت نوار هشدار و پنجره EVENTS است.
- `AuditLogModel` آخرین ۵۰۰ رویداد را برای پنجره EVENTS نگه می‌دارد و هر کدام را به ثبت واقعی می‌فرستد (الگوی Decorator).
- `IAlarmSound` رابط صدا است. `QtAlarmSound` ملودی‌های ساخته‌شده را در یک پوشه موقت می‌نویسد و با QSoundEffect تکرار می‌کند، چون این کلاس فقط WAV پخش می‌کند. با گزینه `--no-sound` از `NullAlarmSound` استفاده می‌شود.
- `FileAuditLog` (کتابخانه vsm_audit، بدون Qt) برای هر رویداد یک خط CSV می‌نویسد و فوراً روی دیسک می‌برد: `time_utc,event,actor,code,priority,detail`. تابع `isOk()` فایلی را که باز نمی‌شود گزارش می‌کند.
- فایل‌های QML: Main.qml، ConnectionBar.qml، AlarmBanner.qml، EventLogPopup.qml، WaveformPanel.qml، NumericTile.qml و Theme.qml (singleton رنگ‌ها).

## ۵-۶ موتور شبیه‌ساز (vsm::simulator)

- `SyntheticPatient` نوار قلب را از موج‌های گاوسی P، Q، R، S و T با کمی نوسان خط پایه می‌سازد و HR و SpO2 را به سمت مقدار هدف حرکت می‌دهد (۳ ضربه در دقیقه بر ثانیه و ۰٫۵ درصد بر ثانیه).
- `Scenario` فهرستی از مراحل زمان‌دار با PatientTargets است؛ `makeScenario(name)` شش سناریوی داخلی را می‌سازد. سناریو پس از آخرین مرحله دوباره از اول شروع می‌شود.
- `DeviceSimulator` هر ۴۰ میلی‌ثانیه `tick()` را اجرا می‌کند: نمونه‌های ECG از تیک قبل را تولید می‌کند، یک فریم ECG می‌فرستد و هر یک ثانیه مقادیر عددی و ضربان حیات را می‌فرستد. حالت سناریو و حالت دستی، کابل جداشده (فریم‌ها شماره می‌خورند ولی فرستاده نمی‌شوند) و احتمال خرابی را پشتیبانی می‌کند و آمار TransmitStatistics را نگه می‌دارد. ساعت به‌صورت `std::function<qint64()>` تزریق می‌شود.
- `IByteSink` رابط خروجی است. `TcpBroadcastSink` (یک QTcpServer روی 127.0.0.1 که به همه مانیتورهای وصل‌شده می‌فرستد) و `SerialSink` (8N1، فقط نوشتن، و دور ریختن داده وقتی بیش از ۶۴ کیلوبایت در صف باشد) آن را پیاده می‌کنند. `createDefaultSink(OutputSettings)` یکی را انتخاب می‌کند.

## ۵-۷ برنامه‌های شبیه‌ساز

- خط فرمان: `tools/simulator/cli/main.cpp` با گزینه‌های --scenario، --port، --serial، --baud، --rate و --corrupt؛ تغییر مراحل و وضعیت خروجی را چاپ می‌کند.
- گرافیکی: `PatientSimulatorController` مدل نمای پنل جلو است (روشن/خاموش، سناریو، مقادیر هدف، کلیدهای حسگر، خروجی، تزریق خطا، آمار). ماژول QML به نام VitalSigns.PatientSimulator شامل Main.qml، DeviceCase.qml، LcdScreen.qml، ParameterGroup.qml، RotaryKnob.qml، PanelKey.qml، Led.qml، GroupFrame.qml، OutputPanel.qml و Panel.qml (singleton رنگ‌ها) است. پنل در اندازه طراحی ۱۲۰۰ در ۷۷۰ چیده شده و به‌صورت یکجا مقیاس می‌خورد.

# ۶- پروتکل دستگاه (خلاصه)

همه فیلدهای چندبایتی little-endian هستند. جزئیات در docs/PROTOCOL.md آمده است.

| آفست | اندازه | فیلد | توضیح |
|---|---|---|---|
| 0 | 2 | همگام‌سازی | 0xAA 0x55 |
| 2 | 1 | نسخه | 1 |
| 3 | 1 | نوع پیام | 0x01 ضربان حیات، 0x10 مقادیر عددی، 0x20 موج ECG |
| 4 | 2 | شماره ترتیب | به ازای هر فریم یکی زیاد می‌شود و پس از 65535 به 0 برمی‌گردد |
| 6 | 2 | طول داده | N، حداکثر 1024 |
| 8 | N | داده | بسته به نوع پیام |
| 8+N | 2 | CRC | CRC-16/CCITT-FALSE روی بایت‌های 2 تا 8+N-1 |

قواعد گیرنده: بایت‌های همگام‌سازی را جست‌وجو کن و بایت‌های ردشده را بشمار؛ نسخه نادرست یا طول بیش از ۱۰۲۴ را پیش از انتظار برای داده رد کن؛ در خطای CRC یک بایت را کنار بگذار و دوباره جست‌وجو کن؛ پرش‌های شماره ترتیب و فریم‌های گم‌شده را بشمار؛ انواع پیام ناشناخته را نادیده بگیر؛ داده عددی و ECG هر کدام زمان‌سنج ۳ ثانیه‌ای خودشان را از نو شروع می‌کنند ولی ضربان حیات نه.

# ۷- ساخت

## ۷-۱ با Qt Creator

1. فایل `CMakeLists.txt` سطح بالا را باز کنید.
2. کیت «Desktop Qt 6.12.0 MinGW 64-bit» (یا هر کیت Qt 6.8 به بالا) را انتخاب کنید. preset‌هایی که Qt Creator از CMakePresets.json پیشنهاد می‌کند لازم نیستند.
3. با Ctrl+B بسازید. فایل‌های اجرایی در `<build>/bin` قرار می‌گیرند.
4. برنامه مورد نظر را در انتخاب‌گر کیت (ستون Run) انتخاب کنید: VitalSignsPatientSimulator، VitalSignsMonitor یا VitalSignsSimulator.

> برای اجرای هم‌زمان شبیه‌ساز و مانیتور از داخل Qt Creator، گزینه Preferences > Build & Run > General > «Stop applications before building» را روی «Same Application» بگذارید. با مقدار پیش‌فرض، Qt Creator هنگام ساخت مانیتور شبیه‌ساز را می‌بندد.

## ۷-۲ از خط فرمان (Git Bash و MinGW)

```
export QTDIR=C:/Qt/6.12.0/mingw_64
export PATH="$QTDIR/bin:/c/Qt/Tools/mingw1310_64/bin:/c/Qt/Tools/Ninja:/c/Qt/Tools/CMake_64/bin:$PATH"
cmake --preset mingw-debug
cmake --build --preset mingw-debug
ctest --preset mingw-debug
```

preset‌ها: `mingw-debug` (Ninja، ویندوز)، `windows-msvc` با preset‌های ساخت `windows-debug` و `windows-release` (Visual Studio 2022) و `linux-debug` (Ninja، لینوکس). همه از متغیر محیطی QTDIR به‌عنوان CMAKE_PREFIX_PATH استفاده می‌کنند.

## ۷-۳ گزینه‌های CMake

| گزینه | پیش‌فرض | اثر |
|---|---|---|
| VSM_BUILD_TESTS | ON | ساخت مجموعه‌های GoogleTest (GoogleTest هنگام پیکربندی دانلود می‌شود) |
| VSM_BUILD_SIMULATOR | ON | ساخت موتور شبیه‌ساز و هر دو برنامه شبیه‌ساز |
| VSM_WARNINGS_AS_ERRORS | OFF | افزودن -Werror یا /WX به هدف‌های خود پروژه |

سیاست هشدارها در `cmake/CompilerWarnings.cmake` تعریف شده است: GCC و Clang از -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wconversion -Wsign-conversion -Wold-style-cast -Woverloaded-virtual و MSVC از /W4 /permissive- /utf-8 /Zc:__cplusplus استفاده می‌کنند. کد شخص ثالث هرگز با این پرچم‌ها کامپایل نمی‌شود.

# ۸- آزمون

## ۸-۱ اجرای آزمون‌ها

دستور `ctest --preset mingw-debug` (یا «Run All Tests» در Qt Creator) را اجرا کنید. پیش از هر commit باید هر ۱۴۰ آزمون موفق باشند.

| فایل اجرایی آزمون | مجموعه | تعداد | آنچه ثابت می‌کند |
|---|---|---|---|
| vsm_unit_tests | Crc16 | 3 | مقدار کنترلی استاندارد، ورودی خالی، تشخیص تغییر یک بیت |
| vsm_unit_tests | FrameCodec | 3 | سرآیند و CRC به ترتیب little-endian، داده خالی، رد داده بیش از حد بزرگ |
| vsm_unit_tests | FrameDecoder | 10 | چند فریم در یک تکه، ورودی بایت به بایت، داده بی‌معنی، CRC نادرست، نسخه نادرست، طول بیش از حد، حافظه محدود، بازنشانی |
| vsm_unit_tests | Messages | 9 | رفت و برگشت هر نوع پیام، مقادیر و پرچم‌های نامعتبر، اندازه نادرست داده، نادیده گرفتن نوع ناشناخته |
| vsm_unit_tests | DefaultAlarmsTest، AlarmEvaluator | 11 | حدود انحصاری، جایگزینی هشدار متوسط با بحرانی، مرتب‌سازی، مقادیر ناموجود، هشدارهای حسگر، سرکوب هنگام قطع ارتباط، قاعده سفارشی |
| vsm_unit_tests | AlarmLimits | 3 | معتبر بودن مقادیر پیش‌فرض، ترتیب حدود HR و SpO2 |
| vsm_unit_tests | SweepBuffer | 4 | شروع خالی، چرخش، پاک کردن، رد ظرفیت صفر |
| vsm_unit_tests | FrameDecoderFuzz | 4 | بایت‌های تصادفی، تکه‌های تصادفی و بایت‌های خراب (بذر ثابت): بدون خرابی، حافظه محدود، هرگز فریم نادرست |
| vsm_unit_tests | AlarmManagerTest، AuditEvents | 26 | ماندگاری، تأیید، توقف صدا و پایان زودهنگام آن، مرتب‌سازی، اولویت صدا، ثبت رویداد، رویداد بحرانی کوتاه |
| vsm_unit_tests | AlarmTone، Wav، AlarmToneByPriority | 16 | تعداد پالس، طول، پایان مناسب تکرار، بدون اشباع، سرآیند WAV و ترتیب بایت‌ها |
| vsm_unit_tests | AuditFormat، FileAuditLogTest | 12 | زمان UTC، نقل‌قول CSV، سرآیند یک‌باره، نوشتن فوری، افزودن، فایل غیرقابل نوشتن |
| vsm_device_tests | DeviceSessionTest | 18 | نشست با FakeTransport: تبدیل مقادیر، واحد ECG، فریم‌های تکه‌تکه، پرش‌ها و خطاهای CRC، چرخش شماره ترتیب، زمان‌سنج نگهبان داده عددی و ECG، مقادیر ناممکن، اتصال مجدد |
| vsm_app_tests | MonitorViewModelTest | 10 | مدل نمای واقعی و زنجیره هشدار با ترابر و صدای جعلی: پاک شدن داده کهنه، صدا، ماندگاری، توقف صدا، قطع اتصال، ثبت رویداد |
| vsm_simulation_tests | DeviceSimulatorTest | 10 | موتور با FakeSink و ساعت جعلی: نرخ نمونه‌برداری، مقادیر عددی، تغییر تدریجی HR، حسگر جدا، کابل، خرابی فریم، سناریوها، خاموش کردن |
| CTest | traceability_check | 1 | هماهنگی docs/medical با کد و آزمون‌ها |

## ۸-۲ بدل‌های آزمون

- `FakeTransport` (در tests/device/FakeTransport.h) جای سوکت را می‌گیرد: آزمون بایت‌ها و تغییر وضعیت را وارد می‌کند و خروجی نشست را بررسی می‌کند.
- `FakeSink` (در DeviceSimulatorTest.cpp) بایت‌هایی را که شبیه‌ساز می‌فرستد جمع می‌کند.
- یک ساعت جعلی (lambda ای که یک متغیر را برمی‌گرداند) `DeviceSimulator::tick()` را می‌راند، پس آزمون یک سناریوی ۲۱ ثانیه‌ای بدون sleep در چند میلی‌ثانیه اجرا می‌شود.
- آزمون‌های device و simulation فایل مشترک `tests/device/main.cpp` را دارند که QCoreApplication لازم برای سیگنال‌های صف‌شده و زمان‌سنج‌ها را می‌سازد. سیگنال‌ها به‌جای QSignalSpy با lambda شمرده می‌شوند، پس وابستگی به QtTest لازم نیست.

## ۸-۳ نوشتن آزمون جدید

1. یک TEST() یا TEST_F() به فایل مناسب در `tests/` اضافه کنید (یا فایل جدیدی بسازید و آن را در tests/CMakeLists.txt فهرست کنید).
2. برای سخت‌افزار و زمان از بدل‌ها استفاده کنید؛ هرگز در آزمون sleep نکنید.
3. نام آزمون را از روی رفتار بگذارید، مثلاً `ReportsLinkLostAfterWatchdogTimeout`.
4. بسازید و ctest را اجرا کنید؛ آزمون جدید خودکار شناسایی می‌شود (gtest_discover_tests).

## ۸-۴ یکپارچه‌سازی پیوسته

فایل `.github/workflows/ci.yml` همه چیز را می‌سازد و همه آزمون‌ها را روی ویندوز (MinGW) و لینوکس (GCC) اجرا می‌کند و هشدارهای کامپایلر را خطا حساب می‌کند. این فایل از Qt 6.8.3، قدیمی‌ترین نسخه پشتیبانی‌شده، استفاده می‌کند تا توسعه روی Qt جدیدتر بی‌صدا حداقل نسخه را بالا نبرد. روی هر pull request و هر push به main اجرا می‌شود. MSVC هنوز پوشش داده نشده است.

فایل‌های QML آزمون خودکار ندارند؛ آن‌ها را با شبیه‌ساز دستی بررسی کنید (راهنمای کاربری، بخش ۷).

## ۸-۵ اسناد به سبک نرم‌افزار پزشکی

پوشه `docs/medical/` نیازمندی‌های نرم‌افزار، تحلیل ریسک و ماتریس ردیابی‌پذیری را به سبک IEC 62304 و ISO 14971 دارد. این‌ها تمرین آموزشی‌اند و ادعای انطباق ندارند. وقتی رفتاری را تغییر می‌دهید آن‌ها را هم به‌روز کنید: هر نیازمندی کد و آزمون خودش را نام می‌برد و `tools/check-traceability.ps1` (آزمون CTest به نام `traceability_check`) با پیوند شکسته خطا می‌دهد. این اسکریپت وجود پیوندها را بررسی می‌کند، نه این‌که آزمون واقعاً نیازمندی را ثابت می‌کند.

# ۹- توسعه سامانه

## ۹-۱ افزودن قاعده هشدار

1. اگر یک حد یا یک شرط ساده کافی است، یک LimitAlarmRule یا ConditionAlarmRule با یک کد پایدار جدید در `makeDefaultAlarmRules()` (فایل src/core/AlarmRules.cpp) اضافه کنید.
2. برای منطق دیگر، کلاسی بنویسید که IAlarmRule::evaluate() را پیاده کند.
3. اگر قاعده منبع جدیدی دارد، AlarmSource را گسترش دهید تا قاعده «یک هشدار برای هر منبع» درست کار کند.
4. آزمون‌ها را به AlarmEvaluatorTest اضافه کنید و جدول هشدار را در docs/ARCHITECTURE.md و راهنمای کاربری به‌روز کنید.

## ۹-۲ افزودن ترابر (مثلاً بلوتوث)

1. رابط ITransport را پیاده کنید (open، close، state، description و سه سیگنال).
2. ConnectionSettings و createDefaultTransport() را گسترش دهید.
3. تنظیمات را به ConnectionBar.qml و یک متد اتصال را به MonitorViewModel اضافه کنید.
4. DeviceSession تغییری نمی‌کند؛ ترابر جدید را مانند FakeTransport با آن بیازمایید.

## ۹-۳ افزودن خروجی شبیه‌ساز

1. رابط IByteSink را پیاده کنید. تابع write() هرگز نباید مسدود شود؛ اگر گیرنده عقب ماند، داده را دور بریزید.
2. OutputSettings و createDefaultSink() را گسترش دهید.
3. یک کلید به OutputPanel.qml و یک متد باز کردن به PatientSimulatorController اضافه کنید.

## ۹-۴ افزودن سناریو

یک نام به `scenarioNames()` و مراحل آن را به `makeScenario()` در فایل tools/simulator/engine/Scenario.cpp اضافه کنید. اگر نام بزرگ‌حرف پیش‌فرض برای کلید طولانی است، برچسب کلید را در DeviceCase.qml (scenarioKeyLabels) تعریف کنید و یک آزمون در DeviceSimulatorTest بنویسید.

## ۹-۵ افزودن نوع پیام

1. یک کد نوع جدید انتخاب کنید و داده آن را در docs/PROTOCOL.md مستند کنید.
2. یک struct به Messages.h اضافه کنید، آن را در variant به نام Message بگذارید و toFrame() و parseMessage() را گسترش دهید.
3. آن را از DeviceSimulator بفرستید و در DeviceSession پردازش کنید.
4. موارد آزمون را به MessagesTest اضافه کنید. مانیتورهای قدیمی‌تر نوع جدید را نادیده می‌گیرند (قاعده ۵ گیرنده).

# ۱۰- قواعد کدنویسی

- لایه‌بندی را حفظ کنید: protocol و core بدون Qt می‌مانند و core هرگز protocol را include نمی‌کند.
- نوع‌های واقعی را فقط در فایل‌های main.cpp به هم وصل کنید.
- کلاس‌هایی که با QML_ELEMENT ثبت می‌شوند نباید `final` باشند (Qt از آن‌ها ارث‌بری می‌کند).
- توابع Q_INVOKABLE را `static` نکنید و از `[[nodiscard]]` روی آن‌ها هم پرهیز کنید.
- هر کلاس منطقی جدید آزمون GoogleTest می‌گیرد و ساخت باید بدون هشدار بماند.
- نوع‌های عمومی توضیح `///` به سبک Doxygen دارند؛ توضیحات «چرا» را می‌گویند، نه «چه» را.
- به استانداردهای پزشکی فقط «به سبک» اشاره شود و هرگز ادعای انطباق نشود. شبیه‌ساز باید برچسب «SYNTHETIC SIGNALS ONLY - NOT A MEDICAL DEVICE» را حفظ کند.

# ۱۱- اشکال‌زدایی و رفع مشکل

| مشکل | علت | راه حل |
|---|---|---|
| CMake ماژول Qt6 SerialPort را پیدا نمی‌کند | ماژول Qt Serial Port برای کیت نصب نشده است | با Qt MaintenanceTool نصب کنید (Qt 6.12.0 > Additional Libraries > Qt Serial Port) |
| با اجرای مانیتور از Qt Creator، شبیه‌ساز بسته می‌شود | Qt Creator پیش از ساخت، برنامه‌های همان پروژه را می‌بندد | گزینه «Stop applications before building» را روی «Same Application» بگذارید |
| پیام «QML debugging is enabled» در خروجی | پیام عادی ساخت Debug | اقدامی لازم نیست |
| مانیتور «Connecting ...» یا «retrying» نشان می‌دهد | شبیه‌ساز اجرا نشده، درگاه اشتباه است یا خروجی بسته است | شبیه‌ساز را اجرا کنید، درگاه را بررسی کنید (پیش‌فرض 5555) و در شبیه‌ساز OPEN را بزنید |
| پیام‌های ثبت رویداد زیاد یا کم است | همه دسته‌های vsm.* به‌طور پیش‌فرض فعال‌اند | با QT_LOGGING_RULES فیلتر کنید، مثلاً "vsm.device.session.info=false" |
| برنامه بیرون از Qt Creator اجرا نمی‌شود | فایل‌های DLL کیوت پیدا نمی‌شوند | پوشه bin کیوت را به PATH اضافه کنید یا windeployqt را اجرا کنید |

# ۱۲- کنترل نسخه

- پوشه‌های ساخت، `CMakeLists.txt.user` و `.qtcreator/` را commit نکنید (فایل .gitignore را ببینید).
- هر commit یک تغییر منطقی؛ پیام commit بگوید چه چیزی و چرا تغییر کرده است.
- پیش از commit: ساخت بدون هشدار و اجرای همه آزمون‌ها.

# ۱۳- تولید دوباره این اسناد

اسناد Word در پوشه‌های `Documents/EN` و `Documents/FA` از منابع شبه‌Markdown در `Documents/source/en` و `Documents/source/fa` و به کمک Microsoft Word تولید می‌شوند:

```
powershell -ExecutionPolicy Bypass -File Documents\source\build-docs.ps1
powershell -ExecutionPolicy Bypass -File Documents\source\build-docs.ps1 -Only fa\02-Developer-Guide.md
```

منبع را ویرایش کنید، اسکریپت را اجرا کنید و هم منبع و هم فایل .docx را commit کنید.
