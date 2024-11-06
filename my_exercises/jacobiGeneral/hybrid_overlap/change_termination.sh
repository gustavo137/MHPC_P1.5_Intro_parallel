#!/bin/bash
# Check if exactly two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <current_extension> <new_extension>"
    exit 1
fi

current_ext=$1
new_ext=$2

# Loop through files with the current extension and rename them
for file in *"$current_ext"; do
    # Check if the file exists to avoid errors
    if [ -e "$file" ]; then
        mv "$file" "${file%$current_ext}$new_ext"
    else
        echo "No files found with the extension $current_ext"
        break
    fi
done

echo "Renaming completed."

### to compile and run:
### chmod +x change_name.sh
### ./change_termination.sh .dat .datg

