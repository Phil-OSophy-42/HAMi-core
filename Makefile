.DEFAULT_GOAL := build

current_dir := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))

build:
	sh ./build.sh
.PHONY: build

build-in-docker:
	docker run -it --rm \
		-v $(current_dir):/libvgpu \
		-w /libvgpu \
		-e DEBIAN_FRONTEND=noninteractive \
		nvidia/cuda:12.2.0-devel-ubuntu20.04 \
		sh -c "apt-get -y update && apt-get -y install cmake git && bash ./build.sh"
.PHONY: build-in-docker

dev:
	docker run -it --rm \
		-v $(current_dir):/libvgpu \
		-w /libvgpu \
		--network=host \
		--gpus all \
		-e DEBIAN_FRONTEND=noninteractive \
		nvidia/cuda:12.2.0-devel-ubuntu20.04 \
		bash
.PHONY: dev

dev-cpu:
	docker run -it --rm \
		-v $(current_dir):/libvgpu \
		-w /libvgpu \
		--network=host \
		-e DEBIAN_FRONTEND=noninteractive \
		nvidia/cuda:12.2.0-devel-ubuntu20.04 \
		bash -c "sed -i 's|mirrors.aliyun.com|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
			sed -i 's|security.ubuntu.com|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
			apt-get -y update && \
			apt-get -y install cmake git
			"
.PHONY: dev-cpu
