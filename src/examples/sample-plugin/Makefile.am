# Copyright (c) 2015 Cisco and/or its affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CFLAGS = -Wall
AM_LDFLAGS = -module -shared -avoid-version

vppapitestpluginsdir = ${libdir}/vpp_api_test_plugins
vpppluginsdir = ${libdir}/vpp_plugins

vppapitestplugins_LTLIBRARIES = sample_test_plugin.la
vppplugins_LTLIBRARIES = sample_plugin.la

sample_plugin_la_SOURCES = sample/sample.c sample/node.c \
	sample/sample_plugin.api.h

BUILT_SOURCES = sample/sample.api.h sample/sample.api.json

SUFFIXES = .api.h .api

%.api.h: %.api
	mkdir -p `dirname $@` ; \
	$(CC) $(CPPFLAGS) -E -P -C -x c $^ \
	| vppapigen --input - --output $@ --show-name $@

%.api.json: %.api
	@echo "  JSON APIGEN  " $@ ;				\
	mkdir -p `dirname $@` ;					\
	$(CC) $(CPPFLAGS) -E -P -C -x c $^			\
	| vppapigen --input - --json $@

apidir = $(prefix)/sample/
api_DATA = sample.api.json

noinst_HEADERS =				\
  sample/sample_all_api_h.h			\
  sample/sample_msg_enum.h			\
  sample/sample.api.h

sample_test_plugin_la_SOURCES = sample/sample_test.c sample/sample_plugin.api.h

# Remove *.la files
install-data-hook:
	@(cd $(vpppluginsdir) && $(RM) $(vppplugins_LTLIBRARIES))
	@(cd $(vppapitestpluginsdir) && $(RM) $(vppapitestplugins_LTLIBRARIES))
