rm -rf display-driver.pio.h
rm -rf build/*
cd build
cmake ..
bear -- make
rm -rf ../compile_commands.json
cp compile_commands.json ..
cp display-driver.pio.h ..
cp gps-clock.uf2 /media/pedrohcs8/RPI-RP2
$shell
