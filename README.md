# Bitcoin Clock

**Bitcoin Clock V1**은 ESP32 기반의 작은 Bitcoin 정보 표시 장치입니다. 채굴 및 Stratum 기능은 실행하지 않으며, 최신 블록 높이, BTC/USD·BTC/KRW 가격, 한국 시각을 표시합니다.

## 원본 및 라이선스

이 프로젝트는 [BitMaker-hub/NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2)를 기반으로 Bitcoin Clock 용도에 맞게 수정한 파생 프로젝트입니다. 원본의 MIT 라이선스와 저작권 고지는 [LICENSE](LICENSE)에 보존되어 있습니다.

저장소에는 별도 조건으로 배포되는 라이브러리와 글꼴도 포함되어 있습니다. 해당 항목과 고지 위치는 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)를 확인하세요. 루트 MIT 라이선스가 별도 고지된 제3자 자산의 라이선스를 대체하지 않습니다.

## 지원 보드

현재 지원하는 보드는 **LILYGO T-Display-S3**입니다. PlatformIO 환경 이름은 `Lilygo-T-Display-S3`입니다.

## 화면과 버튼

- T-Display-S3에서는 버튼 1을 눌러 BTC/USD 가격, BTC/KRW 가격, 블록 높이, 한국 시각 화면을 순환합니다. 부팅 시 BTC/USD 가격 화면을 기본으로 표시합니다.
- 블록 높이는 mempool.space에서 가져오며, BTC/USD는 Binance 공개 API, BTC/KRW는 Upbit 공개 API에서 가져옵니다. 두 거래소의 공개 현재가 조회에는 API 키가 필요하지 않습니다.
- BTC/KRW 시세는 원화 기호(₩)와 함께 만원 단위로 반올림해 표시합니다. 예를 들어 화면의 `₩ 11575`는 약 115,750,000원(11,575만원)을 뜻합니다.
- 블록 및 가격 화면은 7개 타일을 고정해서 사용합니다. 값이 더 길어지면 `CURRENT BLOCK`, `BTC USD`/`BTC KRW`, 통화 기호 순서로 보조 표기를 숨겨 숫자 칸을 확보합니다.
- T-Display-S3 시각과 자동 밝기 일정은 설정한 지역 시간대의 현지 시각을 따릅니다. 미국 등 서머타임이 적용되는 지역은 계절에 따라 자동 조정됩니다.
- 가격과 블록 높이는 네트워크가 연결되면 자동으로 갱신됩니다.
- 버튼 2를 짧게 누르면 화면 밝기가 꺼짐 → 25% → 50% → 75% → 100% 순서로 바뀝니다. 길게 누르면 설정을 초기화합니다.
- T-Display-S3 자동 밝기 스케줄은 설정에서 켜거나 끌 수 있으며, 기본값은 꺼짐입니다. 끄면 부팅 밝기가 75%이며 자동 변경 없이 버튼으로 조절할 수 있습니다. 켜면 NTP 동기화 시 현지 시간대의 현재 밝기를 적용한 뒤 평일에는 00:00~05:30 꺼짐, 05:30~07:00 25%, 07:00~09:00 50%, 09:00~11:00 75%, 11:00~17:00 꺼짐, 17:00~24:00 50%로 바뀝니다. 주말은 00:00~05:30 꺼짐, 05:30~07:00 25%, 07:00~09:00 50%, 09:00~18:00 75%, 18:00~24:00 50%입니다. 꺼짐 시간대에 버튼을 누르면 50%로 켜지고, 그 밝기는 다음 스케줄 경계까지 유지됩니다.

## Wi-Fi 설정

처음 설정할 때 장치의 `BitcoinClock` 액세스 포인트에 연결합니다.

- 네트워크 이름: `BitcoinClock`
- 기본 비밀번호: `BitcoinClock`
- 설정 페이지에서 Wi-Fi 네트워크와 지역 시간대를 선택 상자에서 지정하고 자동 밝기 스케줄을 켜거나 끌 수 있습니다. 기본값은 `Asia/Seoul`입니다. 지원 지역은 `Asia/Seoul`, `Asia/Tokyo`, `Asia/Shanghai`, `Asia/Singapore`, `UTC`, `America/New_York`, `America/Chicago`, `America/Denver`, `America/Phoenix`, `America/Los_Angeles`, `America/Anchorage`, `Pacific/Honolulu`, `Europe/London`, `Europe/Paris`, `Australia/Sydney`, `Pacific/Auckland`입니다.
- 저장된 Wi-Fi 연결을 3회 시도한 뒤 실패하면 화면에 안내가 표시되고 `BitcoinClock` 설정용 접속점이 열립니다. Wi-Fi 이름과 비밀번호는 `config.json`이 아니라 WiFiManager가 기기 내부에 저장합니다. `config.json`은 풀 설정과 시간대 등 앱 설정용입니다.
- S3 시간대는 목록에서 선택하며, 펌웨어가 지원하는 지역만 표시됩니다. 시간대 변경을 위해 설정 포털에 다시 들어가면 Wi-Fi 접속 정보도 다시 입력해야 할 수 있습니다.

## 빌드

PlatformIO Core가 설치된 환경에서 다음 명령으로 빌드합니다.

```sh
# T-Display-S3
pio run -e Lilygo-T-Display-S3
```

화면의 펌웨어 버전은 `V1`입니다. 빌드 파일은 선택한 환경의 `.pio/build/<environment>/` 아래에 생성됩니다.

## PlatformIO 없이 펌웨어 플래싱

PlatformIO를 설치하지 않아도 [GitHub Releases](https://github.com/unsoft/bitcoin-clock/releases/latest)에서 보드에 맞는 `*_factory.bin` 파일을 받아 ESP32에 직접 기록할 수 있습니다. `*_factory.bin`은 부트로더, 파티션 테이블, 앱 펌웨어를 포함한 통합 이미지입니다. `*_firmware.bin`은 앱 업데이트용이므로 처음 USB 플래싱할 때 사용하지 마세요.

1. GitHub Releases에서 `Lilygo-T-Display-S3_factory.bin` 파일을 다운로드합니다.
2. USB 케이블로 보드를 연결하고 [Espressif esptool](https://github.com/espressif/esptool)을 설치합니다. Python이 설치되어 있어야 합니다.

   ```sh
   python3 -m pip install esptool
   ```

3. 운영체제에서 보드의 시리얼 포트를 확인합니다. macOS에서는 `/dev/cu.usbmodem...` 또는 `/dev/cu.usbserial...`, Linux에서는 `/dev/ttyACM0` 또는 `/dev/ttyUSB0`, Windows에서는 `COM3` 같은 이름입니다.
4. 터미널에서 다운로드한 파일이 있는 폴더로 이동한 뒤, 보드에 맞는 명령을 실행합니다. 아래의 `PORT`를 실제 포트 이름으로 바꾸세요.

   ```sh
   # LILYGO T-Display-S3 (ESP32-S3)
   python3 -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash 0x0 Lilygo-T-Display-S3_factory.bin
   ```

   Windows에서 `python3` 명령을 찾을 수 없으면 `py -m esptool`을 사용합니다. 업로드가 실패하면 `--baud 115200`으로 낮춰 다시 시도하고, 필요하면 보드의 `BOOT` 버튼을 누른 채 USB를 연결한 후 명령을 실행하세요.

5. 완료되면 보드를 재시작합니다. 전체 플래시를 지우는 별도 명령은 설정과 저장된 Wi-Fi 정보도 삭제하므로, 특별히 초기화할 때가 아니면 실행하지 마세요.
