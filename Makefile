# ShadowWind MUD - Build and Deployment Makefile

REGISTRY := localhost:30500
IMAGE := shadowwind
TAG := latest

.PHONY: all build clean docker-build docker-push k8s-deploy k8s-delete k8s-logs k8s-logs-websockify k8s-status k8s-port-forward deploy

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

# Deploy to k8s
k8s-deploy: docker-push
	kubectl apply -f deploy/k8s/shadowwind.yaml
	kubectl rollout restart deployment/shadowwind-mud -n shadowwind

# Delete from k8s
k8s-delete:
	kubectl delete -f deploy/k8s/shadowwind.yaml --ignore-not-found

# View logs from running pod (MUD container)
k8s-logs:
	kubectl logs -f deployment/shadowwind-mud -c shadowwind -n shadowwind

# View logs from websockify sidecar
k8s-logs-websockify:
	kubectl logs -f deployment/shadowwind-mud -c websockify -n shadowwind

# Check deployment status
k8s-status:
	kubectl get pods,svc,pvc,ingress -n shadowwind

# Port-forward web client for local testing (access at http://localhost:8080)
k8s-port-forward:
	kubectl port-forward svc/shadowwind-mud -n shadowwind 8080:8080

# Full deployment pipeline
deploy: k8s-deploy k8s-status
