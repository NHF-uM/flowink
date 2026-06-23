#!/bin/bash    

PORT=ttyUSB0


# Check if parameter 1 exists
if [ -z "$1" ]; then
    echo "Error: Parameter 1 is required!"
    echo "Usage: $0 <directory> [python_script_args]"
    exit 1
fi

D_DIR="$1"

echo "Build directory: $D_DIR"

# Check if parameter 2 exists, execute py file only if it exists
if [ -n "$2" ]; then    

    case "$2" in

		# 当第二个参数是 b 时执行逻辑
		b)
			echo "Detect param = b, run build operation..."
			cd ~/zephyrproject/zephyr || exit 1

			west build \
				-b flowink_esp32s3/esp32s3/procpu \
				-d ../flowink/build \
				../flowink/${D_DIR} \
				--pristine \
				-- -DBOARD_ROOT=/home/nhf_um/zephyrproject/flowink

			exit 0
				;;
				
		# 当第二个参数是 c 时执行逻辑
        c)
            echo "Detect param = c, run custom operation..."

    		echo "Executing Python script..."
   			python3 ~/zephyrproject/dtree_flowink_esp32s3.py ~/zephyrproject/flowink/

		    exit 0
			;;

        # 当第二个参数是 f 时执行逻辑
        f)
            # Flash
			cd ~/zephyrproject/flowink/build || exit 1

			ESPTOOL_BIN="${ESPTOOL_BIN:-$(command -v esptool)}"
			if [ -z "$ESPTOOL_BIN" ]; then
				echo "esptool is not available in PATH. Activate the Python environment that provides it first."
				exit 1
			fi

			ESPTOOL_PORT="${ESPTOOL_PORT:-/dev/$PORT}"

			FLASH_ARGS=(
				--chip esp32s3
				--port "$ESPTOOL_PORT"
				--baud 921600
				--before default-reset
				--after hard-reset
				write-flash -u
				--flash-mode dio
				--flash-freq 80m
				--flash-size 16MB
				0x0
				zephyr/zephyr.bin
			)

			FLASH_LOG=$(mktemp)
			if "$ESPTOOL_BIN" "${FLASH_ARGS[@]}" 2>"$FLASH_LOG"; then
				rm -f "$FLASH_LOG"
				exit 0
			fi

			if grep -Eqi 'Permission denied|could not open port|port is busy or doesn'\''t exist' "$FLASH_LOG"; then
				echo "Direct access to $ESPTOOL_PORT failed, retrying with sudo."
				sudo -E env "PATH=$PATH" "$ESPTOOL_BIN" "${FLASH_ARGS[@]}"
			else
				cat "$FLASH_LOG" >&2
				rm -f "$FLASH_LOG"
				exit 1
			fi

			rm -f "$FLASH_LOG"

			ls /dev/ttyA*
			ls /dev/ttyU*

			exit 0
            ;;
            
        # 兜底：匹配 f/c 以外的所有参数
        *)
            echo "Error: Unknown parameter '$2', only support [f / c]"
            ;;
    esac

else
    echo "Parameter 2 not provided"
fi

# Build

echo "Detect param = b, run build operation..."
cd ~/zephyrproject/zephyr || exit 1

west build \
	-b flowink_esp32s3/esp32s3/procpu \
	-d ../flowink/build \
	../flowink/${D_DIR} \
	--pristine \
	-- -DBOARD_ROOT=/home/nhf_um/zephyrproject/flowink

# Flash

cd ~/zephyrproject/flowink/build || exit 1

ESPTOOL_BIN="${ESPTOOL_BIN:-$(command -v esptool)}"
if [ -z "$ESPTOOL_BIN" ]; then
	echo "esptool is not available in PATH. Activate the Python environment that provides it first."
	exit 1
fi

ESPTOOL_PORT="${ESPTOOL_PORT:-/dev/$PORT}"

FLASH_ARGS=(
	--chip esp32s3
	--port "$ESPTOOL_PORT"
	--baud 921600
	--before default-reset
	--after hard-reset
	write-flash -u
	--flash-mode dio
	--flash-freq 80m
	--flash-size 16MB
	0x0
	zephyr/zephyr.bin
)

FLASH_LOG=$(mktemp)
if "$ESPTOOL_BIN" "${FLASH_ARGS[@]}" 2>"$FLASH_LOG"; then
	rm -f "$FLASH_LOG"
	exit 0
fi

if grep -Eqi 'Permission denied|could not open port|port is busy or doesn'\''t exist' "$FLASH_LOG"; then
	echo "Direct access to $ESPTOOL_PORT failed, retrying with sudo."
	sudo -E env "PATH=$PATH" "$ESPTOOL_BIN" "${FLASH_ARGS[@]}"
else
	cat "$FLASH_LOG" >&2
	rm -f "$FLASH_LOG"
	exit 1
fi

rm -f "$FLASH_LOG"
