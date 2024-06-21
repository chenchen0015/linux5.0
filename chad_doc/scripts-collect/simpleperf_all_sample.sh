# 本脚用于使用simpleperf采集某一进程的所有线程PMU数据(111个PMU寄存器)。
# Test Command: blackbox/simpleperf_all_sample.sh -p dji_perception -t 0
echo ""
echo "-h for more info"
echo "HOSTNAME=$HOSTNAME"

while getopts "hp:t:" opt;
do
	case $opt in
		h)#help
			echo ""
			echo ""
			echo "Script Help:"
			echo "-p(MUST): Specify DJI Process Name(ONLY ONE!!!)"
			echo "-t(MUST): Delay Time(Seconds). After Delay Time Will Begin Seting.(eg. -t 30)"
			exit 0
			;;
		p)
			SET_PID_NAME=${OPTARG//,/ }
			;;
		t)
			DELAY_TIME=$OPTARG
			echo "DELAY_TIME=$OPTARG"
			;;	
		\?)
			echo "Invalid option: -$OPTARG"
			;;			
	esac
done

#clean
rm -rf /blackbox/perf_log/
mkdir /blackbox/perf_log/
echo "[`date`][ALL Log Clean.]"

if [ 1 ];then
	echo "[`date`][START]"
	sleep $DELAY_TIME 
	for PID_NAME in ${SET_PID_NAME[@]}
		do
			echo "[`date`][$PID_NAME][START]"
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e instructions -e r001 -e r002 -e r003 -e r004 -e r005 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r006 -e r007 -e r008 -e r009 -e r00a -e r00b --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r00c -e r00d -e r00e -e r00f -e r010 -e r011 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r012 -e r013 -e r014 -e r015 -e r016 -e r017 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r018 -e r019 -e r01a -e r01b -e r01c -e r01d --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r01e -e r020 -e r021 -e r022 -e r023 -e r024 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r025 -e r026 -e r029 -e r02a -e r02b -e r02d --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r02f -e r034 -e r035 -e r036 -e r037 -e r038 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r040 -e r041 -e r042 -e r043 -e r044 -e r045 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r050 -e r051 -e r052 -e r053 -e r060 -e r061 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r066 -e r067 -e r070 -e r071 -e r072 -e r073 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r074 -e r075 -e r076 -e r077 -e r078 -e r079 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r07a -e r086 -e r087 -e r0a0 -e r0a2 -e r0c0 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r0c1 -e r0c2 -e r0c3 -e r0c4 -e r0c5 -e r0c6 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r0c7 -e r0c9 -e r0ca -e r0cb -e r0cc -e r0cd --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r0ce -e r0cf -e r0d0 -e r0d1 -e r0d2 -e r0d3 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r0d4 -e r0d5 -e r0d6 -e r0e1 -e r0e2 -e r0e3 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r0e4 -e r0e5 -e r0e6 -e r0e7 -e r0e8 -e r0e9 --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			simpleperf stat -p `pgrep $PID_NAME` --per-thread -e cpu-cycles -e r0ea -e r0eb -e r0ec --duration 5 >> /blackbox/perf_log/$PID_NAME.log
			echo "[`date`][$PID_NAME][FINISHED]"
	done
fi
