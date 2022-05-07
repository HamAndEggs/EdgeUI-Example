#!/bin/bash
BASE_URL="http://openweathermap.org/img/wn/"

ICONS=(
    "01d"
    "01n"
    "02d"
    "02n"
    "03d"
    "03n"
    "04d"
    "04n"
    "09d"
    "09n"
    "10d"
    "10n"
    "11d"
    "11n"
    "13d"
    "13n"
    "50d"
    "50n"
)

for t in ${ICONS[@]}; do
#    wget "$BASE_URL$t.png"
#    wget "$BASE_URL$t@2x.png"
    wget "$BASE_URL$t@4x.png" -O "$t.png" 
done
