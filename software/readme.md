
改編自 https://github.com/kilograham/rp2040-doom

感謝kilograham做了絕大部分的貢獻
他把doom源碼做了很大的改寫
讓doom可以在rp2040(樹莓派pico)上運行
我只是修改成可以在LCD(ILI9341)上顯示

我在這裡只有放我改過的源碼
並且在修改的地方插入//dahai這樣的記號
主要的修改邏輯是
1.關掉用不到或是衝突的GPIO
2.將原本的scanvideo輸出關閉，改成將緩衝區資料送到LCD

