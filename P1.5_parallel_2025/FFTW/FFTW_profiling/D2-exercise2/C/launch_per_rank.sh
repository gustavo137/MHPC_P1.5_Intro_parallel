#!/bin/bash

# Get the rank of the current MPI process
# RANK=${SLURM_PROCID} # This is global MPI rank, not local
RANK=${SLURM_LOCALID}

# Optional safety check
if [ -z "$RANK" ]; then
  echo "SLURM_LOCALID not set; this script should be run under srun."
  exit 1
fi

# For Perf
FREQ="${PERF_FREQ:-900}"  # samples/seconds 

# output directory per job 
OUTDIR="${PERF_OUTDIR:-$SCRATCH/diffusion/perf_${SLURM_JOB_ID}}" 
mkdir -p "$OUTDIR" 
umask 007   # files created with 600 permissions    

# Unique filename per rank & node  
OUTFILE="${OUTDIR}/perf.data.rank${SLURM_PROCID}" 

 # common perf args  
common_args=(   
       -F "$FREQ"
       --output="$OUTFILE"
       --call-graph dwarf 
       --delay 10 
       -e cycles:u,instructions:u
)

# Launch the binary
exec perf record "${common_args[@]}" -- ./diffusion.x 
