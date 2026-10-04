# Bitcoin Clock

**Bitcoin Clock V1**은 ESP32 기반의 작은 Bitcoin 정보 표시 장치입니다. 채굴 및 Stratum 기능은 실행하지 않으며, 최신 블록 높이, BTC/USD·BTC/KRW 가격, 한국 시각을 표시합니다.

## 원본 및 라이선스

이 프로젝트는 [BitMaker-hub/NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2)를 기반으로 Bitcoin Clock 용도에 맞게 수정한 파생 프로젝트입니다. 원본의 MIT 라이선스와 저작권 고지는 [LICENSE](LICENSE)에 보존되어 있습니다.

저장소에는 별도 조건으로 배포되는 라이브러리와 글꼴도 포함되어 있습니다. 해당 항목과 고지 위치는 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)를 확인하세요. 루트 MIT 라이선스가 별도 고지된 제3자 자산의 라이선스를 대체하지 않습니다.

## 대상 보드

Bitcoin Clock 화면은 **LILYGO TTGO T-Display 1.14 (V1)** 과 **LILYGO T-Display-S3**를 지원합니다. 두 보드의 펌웨어는 각각 `TTGO-T-Display` 및 `Lilygo-T-Display-S3` 환경으로 빌드합니다.

## 화면과 버튼

- T-Display-S3에서는 버튼 1을 눌러 BTC/USD 가격, BTC/KRW 가격, 블록 높이, 한국 시각 화면을 순환합니다. 부팅 시 BTC/USD 가격 화면을 기본으로 표시합니다.
- 블록 높이는 mempool.space에서 가져오며, BTC/USD는 Binance 공개 API, BTC/KRW는 Upbit 공개 API에서 가져옵니다. 두 거래소의 공개 현재가 조회에는 API 키가 필요하지 않습니다.
- BTC/KRW 시세는 원화 기호(₩)와 함께 만원 단위로 반올림해 표시합니다. 예를 들어 화면의 `₩ 11575`는 약 115,750,000원(11,575만원)을 뜻합니다.
- 블록 및 가격 화면은 7개 타일을 고정해서 사용합니다. 값이 더 길어지면 `CURRENT BLOCK`, `BTC USDT`, `$` 순서로 보조 표기를 숨겨 숫자 칸을 확보합니다.
- T-Display-S3 시각은 NTP 동기화 후 한국 표준시(UTC+9)로 표시됩니다.
- 가격과 블록 높이는 네트워크가 연결되면 자동으로 갱신됩니다.
- 버튼 2를 짧게 누르면 화면 밝기가 꺼짐 → 25% → 50% → 75% → 100% 순서로 바뀝니다. 길게 누르면 설정을 초기화합니다.
- T-Display-S3 밝기는 NTP 동기화 시 현재 시간대에 맞춰 설정된 뒤, 평일에는 00:00~05:30 꺼짐, 05:30~08:00 25%, 08:00~11:00 50%, 11:00~17:00 꺼짐, 17:00~24:00 50%로 바뀝니다. 주말은 00:00~05:30 꺼짐, 05:30~08:00 25%, 08:00~24:00 50%입니다. 버튼으로 중간에 변경한 밝기는 다음 시간대 시작까지 유지됩니다.

## Wi-Fi 설정

처음 설정할 때 장치의 `BitcoinClock` 액세스 포인트에 연결합니다.

- 네트워크 이름: `BitcoinClock`
- 기본 비밀번호: `BitcoinClock`
- 설정 페이지에서 Wi-Fi 네트워크와 시간대를 지정합니다.
- 저장된 Wi-Fi 연결을 3회 시도한 뒤 실패하면 화면에 안내가 표시되고 `BitcoinClock` 설정용 접속점이 열립니다. Wi-Fi 이름과 비밀번호는 `config.json`이 아니라 WiFiManager가 기기 내부에 저장합니다. `config.json`은 풀 설정과 시간대 등 앱 설정용입니다.

## 빌드

PlatformIO Core가 설치된 환경에서 보드에 맞는 명령으로 빌드합니다.

```sh
# T-Display V1
pio run -e TTGO-T-Display

# T-Display-S3
pio run -e Lilygo-T-Display-S3
```

화면의 펌웨어 버전은 `V1`입니다. 빌드 파일은 선택한 환경의 `.pio/build/<environment>/` 아래에 생성됩니다.
