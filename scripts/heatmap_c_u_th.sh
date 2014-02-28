#!/bin/bash

MAKE=make

unm=$(uname -n);
if [ $unm = "ol-collab1" ];
then
    MAKE=gmake
fi;

if [ $# -eq 0 ];		# pass any param to avoid compilation
then
    INIT=one GRANULARITY=GLOBAL_LOCK $MAKE -k tas
    INIT=one $MAKE -k tas
    INIT=one $MAKE -k lockfree
fi

inits="256 1024 2048 8192 65536"
duration=1000;

source ./scripts/heatmap.config
source ./scripts/executables.config

for initial in ${inits}
do
range=$((2*${initial}));

echo "## initial: $initial";

unm=$(uname -n);
rm data/${unm}_*_heatmap_uc_*${initial}.csv
echo '#  ll';
./scripts/heatmap_avg.sh ${lb_ll} ${lf_ll} u c -i${initial} -r${range} -d$duration | tee data/${unm}_ll_heatmap_uc_${initial}.csv
cp data/temp1.txt data/${unm}_ll_heatmap_uc_lb_${initial}.csv
cp data/temp2.txt data/${unm}_ll_heatmap_uc_lf_${initial}.csv
cp data/temp3.txt data/${unm}_ll_heatmap_uc_frac_${initial}.csv
cp data/scal_temp.txt data/${unm}_ll_heatmap_uc_scal_${initial}.csv
cp data/scal_data.txt data/${unm}_ll_heatmap_uc_scal_frac_${initial}.csv
echo '#  ht';
./scripts/heatmap_avg.sh ${lb_ht} ${lf_ht} u c -i${initial} -r${range} -d$duration | tee data/${unm}_ht_heatmap_uc_${initial}.csv
cp data/temp1.txt data/${unm}_ht_heatmap_uc_lb_${initial}.csv
cp data/temp2.txt data/${unm}_ht_heatmap_uc_lf_${initial}.csv
cp data/temp3.txt data/${unm}_ht_heatmap_uc_frac_${initial}.csv
cp data/scal_temp.txt data/${unm}_ht_heatmap_uc_scal_${initial}.csv
cp data/scal_data.txt data/${unm}_ht_heatmap_uc_scal_frac_${initial}.csv
echo '#  sl';
./scripts/heatmap_avg.sh ${lb_sl} ${lf_sl} u c -i${initial} -r${range} -d$duration | tee data/${unm}_sl_heatmap_uc_${initial}.csv
cp data/temp1.txt data/${unm}_sl_heatmap_uc_lb_${initial}.csv
cp data/temp2.txt data/${unm}_sl_heatmap_uc_lf_${initial}.csv
cp data/temp3.txt data/${unm}_sl_heatmap_uc_frac_${initial}.csv
cp data/scal_temp.txt data/${unm}_sl_heatmap_uc_scal_${initial}.csv
cp data/scal_data.txt data/${unm}_sl_heatmap_uc_scal_frac_${initial}.csv
echo '#  bst';
./scripts/heatmap_avg.sh ${lb_bst} ${lf_bst} u c -i${initial} -r${range} -d$duration | tee data/${unm}_bst_heatmap_uc_${initial}.csv
cp data/temp1.txt data/${unm}_bst_heatmap_uc_lb_${initial}.csv
cp data/temp2.txt data/${unm}_bst_heatmap_uc_lf_${initial}.csv
cp data/temp3.txt data/${unm}_bst_heatmap_uc_frac_${initial}.csv
cp data/scal_temp.txt data/${unm}_bst_heatmap_uc_scal_${initial}.csv
cp data/scal_data.txt data/${unm}_bst_heatmap_uc_scal_frac_${initial}.csv
done
