#!/bin/bash

ds=ot;
to_move=optik_test;

ub="./bin/$(uname -n)";
uo="scripts/ppopp/data";

do_compile=1;
set_cpu=0;

skip=$#;

algos=( ${ub}/optik_test2 ${ub}/optik_test0 ${ub}/optik_test1 );
repetitions=11;
duration=5000;

params_i=( 1 );
params_u=( 0 );
np=${#params_i[*]};

cores=ppopp;


cores_backup=$cores;
. ./scripts/config;

nc=$(echo "$cores" | wc -w);
dur_s=$(echo $duration/1000 | bc -l);
na=${#algos[@]};

dur_tot=$(echo "$na*$np*$nc*$repetitions*$dur_s" | bc -l);

printf "#> $na algos, $np params, $nc cores, $repetitions reps of %.2f sec = %.2f sec\n" $dur_s $dur_tot;
printf "#> = %.2f hours\n" $(echo $dur_tot/3600 | bc -l);

if [ $skip -eq 0 ];
then
    printf "   Continue? [Y/n] ";
    read cont;
    if [ "$cont" = "n" ];
    then
	exit;
    fi;
fi;

cores=$cores_backup;
algos_str="${algos[@]}";

if [ $do_compile -eq 1 ];
then
    ctarget=${ds}ppopp;
    cflags="SET_CPU=$set_cpu";
    echo "----> Compiling" $ctarget " with flags:" $cflags;
    make $ctarget $cflags >> /dev/null;
    if [ $? -eq 0 ];
    then
	echo "----> Success!"
    fi;
    echo "----> Moving binaries to $ub";
    mkdir $ub &> /dev/null;
    mv bin/${to_move}* $ub;
    if [ $? -eq 0 ];
    then
	echo "----> Success!"
    fi;
fi;


for ((i=0; i < $np; i++))
do
    initial=${params_i[$i]};
    update=${params_u[$i]};
    range=$initial;
    out="$unm.${ds}.i$initial.u$update.dat"
    echo "### params -i$initial -r$range -u$update / keep $keep of reps $repetitions of dur $duration" | tee ${uo}/$out;

    ./scripts/scalability_optik.sh $cores $repetitions "$algos_str" -d$duration -i$initial -r$range -u$update \
				 | tee -a ${uo}/$out;
done;
