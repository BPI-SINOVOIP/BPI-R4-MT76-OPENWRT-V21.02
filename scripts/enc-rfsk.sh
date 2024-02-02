#!/bin/bash
# use roe-key to encrypt rootfs-key, and generate fit-secret
usage() {
	printf "Usage: %s -s build_dir -d key_dir -f fit" "$(basename "$0")"
	printf "encrypt rootfs key: [-k roe_key] "
	printf "generate FIT-secret: [-k roe_key -c config_name]\n"
	printf "\n\t-c ==> config name with signature node"
	printf "\n\t-d ==> key_directory"
	printf "\n\t-f ==> fit image file"
	printf "\n\t-k ==> roe key"
	printf "\n\t-s ==> build directory"
	exit 1
}

hex2bin() {
	$PERL -e 'print pack "H*", <STDIN>'
}

bin2hex() {
	od -An -t x1 -w128 | sed "s/ //g"
}

hkdf() {
	local key=$1
	local info=$2
	local salt=$3
	local k_derived=$(${OPENSSL} kdf -keylen 32 -kdfopt digest:SHA2-256 \
		-kdfopt hexkey:$(cat ${key} | bin2hex) \
		-kdfopt hexsalt:$salt \
		-kdfopt info:$info HKDF | sed "s/://g")
	echo $k_derived
}

gen_aes_key() {
	out=$1
	$OPENSSL rand -out $out 32
}

aes_cbc_enc() {
	local in=$1
	local out=$2
	local key=$3
	local iv=$(${OPENSSL} rand -hex 16)
	$OPENSSL enc -e -aes-256-cbc -in $in -out ${out}.tmp -K $key -iv $iv

	echo -n $iv | hex2bin > $out
	cat ${out}.tmp >> $out
	rm ${out}.tmp
}

aes_gcm_enc() {
	local in=$1
	local out=$2
	local key=$3
	local aad=$4
	local iv=$(${OPENSSL} rand -hex 12)
	$AESGCM -e -i $in -o $out -k $key -n $iv -a $aad
}

# encrypt data with AES
# encrypted-data-format:
# -----------------------------------------------
# | salt | iv | k-tempx.enc | iv | tag | in.enc |
# -----------------------------------------------
enc_data() {
	local k_temp=$1
	local in=$2
	local out=$3
	local info=$4
	local salt=$(${OPENSSL} rand -hex 16)

	echo -n $salt | hex2bin > $out

	# encrypt k-tempx
	aes_cbc_enc $k_temp ${out}.tmp $(hkdf ${ROE_KEY}.key ${info} ${salt})

	cat ${out}.tmp >> $out

	aad=$(cat ${out} | bin2hex)
	# encrypt in
	aes_gcm_enc $in ${out}.tmp $(cat $k_temp | bin2hex) $aad

	cat ${out}.tmp >> $out
	rm ${out}.tmp
}

# generate FIT-secret and insert back into FIT
gen_fit_secret() {
	echo "Generating fit-secret"

	if [ ! -f "${FIT}" ]; then
		printf "%s not found\n" "${FIT}"
		exit 1
	fi

	SECRETS_DIR=$BUILD_DIR/fit-secrets
	if [ ! -d "${SECRETS_DIR}" ]; then
		mkdir -p $SECRETS_DIR
	fi

	LD_LIBRARY_PATH=${LIBFDT_PATH} \
	$FDTGET $FIT /configurations/$CONFIG/signature value -t bi > \
		$SECRETS_DIR/$FIT_NAME-signature.tmp || exit 1

	echo -n $(cat ${SECRETS_DIR}/${FIT_NAME}-signature.tmp) | xargs printf "%02x" | \
		hex2bin > $SECRETS_DIR/$FIT_NAME-signature.raw

	$OPENSSL dgst -sha256 -binary -out $SECRETS_DIR/$FIT_NAME-signature.hash \
		$SECRETS_DIR/$FIT_NAME-signature.raw || exit 1

	gen_aes_key $KEY_DIR/${TEMP2_KEY_NAME}.key

	enc_data $KEY_DIR/${TEMP2_KEY_NAME}.key \
		 $SECRETS_DIR/$FIT_NAME-signature.hash \
		 $SECRETS_DIR/$FIT_NAME-secret.enc \
		 fit-secret

	LD_LIBRARY_PATH=${LIBFDT_PATH} \
	$FDTPUT -c -p $FIT /fit-secrets/$CONFIG || exit 1
	LD_LIBRARY_PATH=${LIBFDT_PATH} \
	$FDTPUT $FIT /fit-secrets/$CONFIG algo -t s "sha256"
	LD_LIBRARY_PATH=${LIBFDT_PATH} \
	$FDTPUT $FIT /fit-secrets/$CONFIG data -t x \
		$(cat ${SECRETS_DIR}/${FIT_NAME}-secret.enc | bin2hex | \
		sed 's/ //g; s/.\{8\}/0x& /g; s/.$//g')
}

# encrypt rootfs key
enc_rfsk() {
	echo "Encrypting rootfs key"

	gen_aes_key $KEY_DIR/${ROOTFS_KEY_NAME}.key
	gen_aes_key $KEY_DIR/${TEMP1_KEY_NAME}.key

	enc_data $KEY_DIR/${TEMP1_KEY_NAME}.key \
		 $KEY_DIR/${ROOTFS_KEY_NAME}.key \
		 $BUILD_DIR/${FIT_NAME}-rfsk.enc \
		 k-rootfs
}

while getopts "c:d:f:k:s:" OPTION
do
	case $OPTION in
	c ) CONFIG=$OPTARG;;
	d ) KEY_DIR=$OPTARG;;
	f ) FIT=$OPTARG;;
	k ) ROE_KEY=$OPTARG;;
	s ) BUILD_DIR=$OPTARG;;
	* ) echo "Invalid option passed to '$0' (options:$*)"
	usage;;
	esac
done

if [ ! -d "${KEY_DIR}" ]; then
	echo "key directory not found"
	usage;
fi

if [ ! -d "${BUILD_DIR}" ]; then
	echo "build directory not found"
	usage;
fi

if [ -z "${FIT}" ]; then
	echo "FIT name is empty"
	usage;
fi

if [ -z "${BIN}" ]; then
	echo "bin directory not found"
	exit 1
fi

if [ ! -f "${ROE_KEY}".key ]; then
	echo "roe-key not found"
	usage;
fi

OPENSSL=$BIN/openssl-3
AESGCM=$BIN/aesgcm
PERL=$BIN/perl
FDTGET=$BIN/fdtget
FDTPUT=$BIN/fdtput

FIT_NAME=$(basename ${FIT} | sed "s/\.[^.]*$//g")
ROOTFS_KEY_NAME=${FIT_NAME}-rootfs-key
TEMP1_KEY_NAME=${FIT_NAME}-temp1-key
TEMP2_KEY_NAME=${FIT_NAME}-temp2-key

if [ ! -z "${CONFIG}" ]; then
	gen_fit_secret;
else
	enc_rfsk;
fi
