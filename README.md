# Bitcoin Clock

**Bitcoin Clock V1**은 ESP32 기반의 작은 Bitcoin 정보 표시 장치입니다. 채굴 및 Stratum 기능은 실행하지 않으며, 최신 블록 높이와 BTC/USDT 가격을 표시합니다.

## 원본 및 라이선스

이 프로젝트는 [BitMaker-hub/NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2)를 기반으로 Bitcoin Clock 용도에 맞게 수정한 파생 프로젝트입니다. 원본의 MIT 라이선스와 저작권 고지는 [LICENSE](LICENSE)에 보존되어 있습니다.

저장소에는 별도 조건으로 배포되는 라이브러리와 글꼴도 포함되어 있습니다. 해당 항목과 고지 위치는 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)를 확인하세요. 루트 MIT 라이선스가 별도 고지된 제3자 자산의 라이선스를 대체하지 않습니다.

## 대상 보드

Bitcoin Clock 화면은 **LILYGO TTGO T-Display 1.14 (V1)** 과 **LILYGO T-Display-S3**를 지원합니다. 두 보드의 펌웨어는 각각 `TTGO-T-Display` 및 `Lilygo-T-Display-S3` 환경으로 빌드합니다.

## 화면과 버튼

- T-Display-S3에서는 버튼을 눌러 블록 높이, BTC/USDT 가격, 로컬 시각 화면을 순환합니다.
- 블록 높이는 mempool.space에서, T-Display-S3 가격은 Binance BTC/USDT 공개 API에서 가져옵니다.
- 블록 및 가격 화면은 7개 타일을 고정해서 사용합니다. 값이 더 길어지면 `CURRENT BLOCK`, `BTC USDT`, `$` 순서로 보조 표기를 숨겨 숫자 칸을 확보합니다.
- T-Display-S3 시각은 NTP 동기화 후 한국 표준시(UTC+9)로 표시됩니다.
- 가격과 블록 높이는 네트워크가 연결되면 자동으로 갱신됩니다.
- 다른 버튼은 화면 백라이트를 전환하며, 길게 누르면 설정을 초기화합니다.

## Wi-Fi 설정

처음 설정할 때 장치의 `BitcoinClock` 액세스 포인트에 연결합니다.

- 네트워크 이름: `BitcoinClock`
- 기본 비밀번호: `BitcoinClock`
- 설정 페이지에서 Wi-Fi 네트워크와 시간대를 지정합니다.

## 빌드

PlatformIO Core가 설치된 환경에서 보드에 맞는 명령으로 빌드합니다.

```sh
# T-Display V1
pio run -e TTGO-T-Display

# T-Display-S3
pio run -e Lilygo-T-Display-S3
```

화면의 펌웨어 버전은 `V1`입니다. 빌드 파일은 선택한 환경의 `.pio/build/<environment>/` 아래에 생성됩니다.
