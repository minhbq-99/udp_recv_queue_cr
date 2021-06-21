#!/bin/sh

set -e

readonly DEV="veth0"
readonly CLIENT_BIN="./client"
readonly SERVER_BIN="./server"

readonly RAND="$(mktemp -u XXXXXX)"
readonly NSPREFIX="ns-${RAND}"
readonly NS1="${NSPREFIX}1"
readonly NS2="${NSPREFIX}2"

ip netns add "${NS1}"
ip netns add "${NS2}"

ip link add "${DEV}" netns "${NS1}" type veth \
  peer name "${DEV}" netns "${NS2}"

ip -netns "${NS1}" link set lo up
ip -netns "${NS1}" link set "${DEV}" up
ip -netns "${NS2}" link set "${DEV}" up

ip -netns "${NS1}" link set dev "${DEV}" address 02:02:02:02:02:02
ip -netns "${NS2}" link set dev "${DEV}" address 06:06:06:06:06:06

ip -netns "${NS1}" addr add 192.168.1.1/24 dev "${DEV}"
ip -netns "${NS2}" addr add 192.168.1.2/24 dev "${DEV}"

ip netns exec "${NS1}" "${SERVER_BIN}" &
ip netns exec "${NS2}" "${CLIENT_BIN}"

cleanup() {
	ip netns del "${NS1}"
	ip netns del "${NS2}"
}

trap cleanup EXIT
