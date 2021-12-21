# Copyright 2020-2021 Laurent Cabaret
# Copyright 2020-2021 Vincent Jacques

function chrones_initialize {
  name=$1
  chrones_filename=$name.chrones.csv
  chrones_stopwatch_index=0
}

function chrones_start_stopwatch {
  function=$1
  label=${2:--}
  index=${3:--}

  # Using 0 for process id and thread id because... it works
  echo "0,0,$(date +%s%N),sw_start,$function,$label,$index" >>$chrones_filename
}

function chrones_stop_stopwatch {
  echo "0,0,$(date +%s%N),sw_stop" >>$chrones_filename
}
