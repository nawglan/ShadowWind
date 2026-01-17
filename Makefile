# ShadowWind MUD - Build and Deployment Makefile

REGISTRY := localhost:30500
IMAGE := shadowwind
WEBCLIENT_IMAGE := shadowwind-webclient
TAG := latest

.PHONY: all build clean docker-build docker-push webclient-build webclient-push k8s-deploy k8s-delete k8s-logs k8s-logs-webclient k8s-status k8s-port-forward deploy format format-check lint

# Default target - build the MUD binary
all: build

# Build the MUD binary locally
build:
	cd src && $(MAKE)

# Clean build artifacts
clean:
	cd src && $(MAKE) clean

# Build Docker image
docker-build:
	docker build -t $(REGISTRY)/$(IMAGE):$(TAG) .

# Push Docker image to local registry
docker-push: docker-build
	docker push $(REGISTRY)/$(IMAGE):$(TAG)

# Build webclient Docker image
webclient-build:
	docker build -t $(REGISTRY)/$(WEBCLIENT_IMAGE):$(TAG) ./webclient

# Push webclient Docker image to local registry
webclient-push: webclient-build
	docker push $(REGISTRY)/$(WEBCLIENT_IMAGE):$(TAG)

# Deploy to k8s
k8s-deploy: docker-push webclient-push
	kubectl apply -f deploy/k8s/shadowwind.yaml
	kubectl rollout restart deployment/shadowwind-mud -n shadowwind

# Delete from k8s
k8s-delete:
	kubectl delete -f deploy/k8s/shadowwind.yaml --ignore-not-found

# View logs from running pod (MUD container)
k8s-logs:
	kubectl logs -f deployment/shadowwind-mud -c shadowwind -n shadowwind

# View logs from webclient sidecar
k8s-logs-webclient:
	kubectl logs -f deployment/shadowwind-mud -c webclient -n shadowwind

# Check deployment status
k8s-status:
	kubectl get pods,svc,pvc,ingress -n shadowwind

# Port-forward web client for local testing (access at http://localhost:8080)
k8s-port-forward:
	kubectl port-forward svc/shadowwind-mud -n shadowwind 8080:8080

# Full deployment pipeline
deploy: k8s-deploy k8s-status

# Code formatting with clang-format
format:
	find src -name '*.c' -o -name '*.h' | xargs clang-format -i

# Check formatting without modifying (CI-friendly)
format-check:
	find src -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror

# Static analysis with cppcheck
lint:
	cppcheck --enable=warning,style,performance --suppress=missingIncludeSystem \
		--suppress=preprocessorErrorDirective:src/zedit.c \
		--error-exitcode=1 src/
