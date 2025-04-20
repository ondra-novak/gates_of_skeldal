#!/bin/bash
temp_file=$(mktemp /tmp/skeldal.XXXXXX.log)
CURDIR=`dirname "$0"`

LD_LIBRARY_PATH=$CURDIR:$LD_LIBRARY_PATH "$CURDIR/skeldal_bin" $* > "$temp_file" 2>&1

exit_code=$?

if [ $exit_code -ne 0 ]; then
    error_message=$(cat "$temp_file")

    if command -v zenity > /dev/null; then
        zenity --warning --no-markup --title="Skeldal ERROR" --text="$error_message"
    elif command -v kdialog > /dev/null; then
        kdialog --title "Skeldal ERROR" --error "$error_message"
    elif command -v xmessage > /dev/null; then
        xmessage -center -file $temp_file
    elif command -v xdg-open > /dev/null; then
        xdg-open "$temp_file"
        sleep 5;
    else
        cat "$temp_file"
    fi
fi
rm $temp_file
exit $exit_code


