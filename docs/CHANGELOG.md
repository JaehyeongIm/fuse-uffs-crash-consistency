CHANGE-001

Title
Seal byte 설계 변경

Reason
NAND Flash 의 특성상 1에서 0으로 바꾸는 것만 가능함

Decision
0xFF 를 erase 상태, 0xFE를 쓰는 중 상태, 0xFC를 쓰기가 완료된 상태로 정의함
기존 설계는 0xFF -> erase 상태, 0x00 -> 쓰는 중상태, 0xFE -> 쓰기가 완료된 상태였음

Impact
SRS
SDD
Test plan
Change log

Date
2026-03-04 
