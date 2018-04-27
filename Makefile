
ANSIBLE_DIR := ansible

all:
	make configure-local
	make update
	make configure
	make init
	make build
	make default-configs
	make deploy
	make run

help:
	@echo "Please use \`make <target>\` where <target> is one of"
	@echo "  configure          to configure the hosts"
	@echo "  init               to initial the template and dissolve dependencies"
	@echo "  build              to build the models"
	@echo "  default-configs    to create default configuration files (saved in \`configurations/config_0\`)"
	@echo "  deploy             to run all models"
	@echo "  clean              to remove temporary data (\`build\` folder)"

configure-local:
	ansible-playbook $(ANSIBLE_DIR)/configure-local.yml --ask-become-pass --connection=local -e ansible_python_interpreter=/usr/bin/python -i ./ansible/inventory/hosts

update:
	ansible-playbook $(ANSIBLE_DIR)/update-inv.yml --connection=local

configure:
	ansible-playbook $(ANSIBLE_DIR)/configure.yml --ask-become-pass -i ./ansible/inventory/hosts

init:
	ansible-playbook $(ANSIBLE_DIR)/init.yml --connection=local -i ./ansible/inventory/hosts

build:
	ansible-playbook $(ANSIBLE_DIR)/build.yml --connection=local -i ./ansible/inventory/hosts

default-configs:
	ansible-playbook $(ANSIBLE_DIR)/default-configs.yml --connection=local -i ./ansible/inventory/hosts

deploy:
	ansible-playbook $(ANSIBLE_DIR)/deploy.yml -i ./ansible/inventory/hosts

run:
	ansible-playbook $(ANSIBLE_DIR)/run.yml -i ./ansible/inventory/hosts

clean:
	ansible-playbook $(ANSIBLE_DIR)/clean.yml -i ./ansible/inventory/hosts