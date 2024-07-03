#!/usr/bin/env bash
#set -x
# Declare an associative array used for error handling
declare -A ERRORS

# Define the "error" values
ERRORS[INVALID_OPTION]=1
ERRORS[INVALID_ARG]=2
ERRORS[OUT_OF_RANGE]=3
ERRORS[NOT_AN_INTEGER]=4
ERRORS[PROGRAMMING_ERROR]=5

# default values for the script arguments
MOUNT_POINT=/mnt/fmap/
H1_SIZE_GB=64
H2_SIZE_GB=512

# Function to print error message and exit
function error_exit {
	echo "Error: $1" >&2
	exit 1
}

# Function to display usage message
function usage() {
	echo "Usage: $0 [options]"
	echo "Options:"
	echo "  -m, --mount_point <mount point>                          Set the mount_point for the H2 heap; eg. /spare2/perpap/fmap/"
	echo "  -n, --h1_size <h1 size>                                  Set h1_size for the size of the H1 heap; eg. 64GB"
	echo "  -f, --h2_size <h2 size>                                  Set h2_size for the size of the H2 heap; eg. 512GB"
	echo "  -h, --help                                               Print help"
	echo
	echo "   Examples:"
	echo
	echo "  ./run_tests.sh                                           Run tests with default values; mount_point:/spare2/perpap/fmap/ H1:64GB H2:512GB"
	echo "  ./run_tests.sh -m /spare2/perpap/fmap/ -n 64 -f 512      Run tests with the values; mount_point:/spare2/perpap/fmap/ H1:64GB H2:512GB"

	return 0 2>/dev/null || exit 0
}

function validate_mount_point() {
	# Check if the mount point is a valid directory
	if [ ! -d "$MOUNT_POINT" ]; then
		error_exit "Mount point '$MOUNT_POINT' is not a valid directory"
	fi

	# Check if the mount point is actually a mount point
	: '
  if ! mountpoint -q "$MOUNT_POINT"; then
		error_exit "'$MOUNT_POINT' is not a mount point"
	fi
  '
}

function validate_device_size() {
	# Get the available size on the mount point in kilobytes
	AVAILABLE_SIZE=$(df -k --output=avail "$MOUNT_POINT" | tail -n 1)

	# Convert the sizes from GB to kilobytes
	H1_SIZE_KB=$((H1_SIZE_GB * 1024 * 1024))
	H2_SIZE_KB=$((H2_SIZE_GB * 1024 * 1024))

	# Calculate the total required size in kilobytes
	TOTAL_REQUIRED_SIZE=$((H1_SIZE_KB + H2_SIZE_KB))

	# Check if there is enough available space
	if [ "$AVAILABLE_SIZE" -lt "$TOTAL_REQUIRED_SIZE" ]; then
		error_exit "Not enough available space on '$MOUNT_POINT' to create heaps of size ${H1_SIZE_GB}GB and ${H2_SIZE_GB}GB"
	fi
}

function parse_script_arguments() {
	local OPTIONS=m:n:f:h
	local LONGOPTIONS=mount_point:,h1_size:,h2_size:,help

	# Use getopt to parse the options
	local PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")

	# Check for errors in getopt
	if [[ $? -ne 0 ]]; then
		return ${ERRORS[INVALID_OPTION]} 2>/dev/null || exit ${ERRORS[INVALID_OPTION]}
	fi

	# Evaluate the parsed options
	eval set -- "$PARSED"

	while true; do
		case "$1" in
		-m | --mount_point)
			MOUNT_POINT="$2"
			validate_mount_point
			shift 2
			;;
		-n | --h1_size)
			H1_SIZE_GB="$2"
			shift 2
			;;
		-f | --h2_size)
			H2_SIZE_GB="$2"
			shift 2
			;;
		-h | --help)
			usage
			exit 0
			;;
		--)
			shift
			break
			;;
		*)
			echo "Programming error"
			return ${ERRORS[PROGRAMMING_ERROR]} 2>/dev/null || exit ${ERRORS[PROGRAMMING_ERROR]} # This will return if sourced, and exit if run as a standalone script
			;;
		esac
	done
}

parse_script_arguments "$@"
validate_device_size

# Loop through all files in the current directory with .bin extension
for file in *.bin; do
	# Check if there are any .bin files
	if [[ -f "$file" ]]; then
		echo "Executing $file"
		# Make the file executable (if not already) and execute it
		chmod +x "$file"
		./"$file" $MOUNT_POINT $H1_SIZE_GB $H2_SIZE_GB
	else
		echo "No .bin files found in the current directory."
		break
	fi
done
