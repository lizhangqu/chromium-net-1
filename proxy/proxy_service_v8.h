// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_PROXY_PROXY_SERVICE_V8_H_
#define NET_PROXY_PROXY_SERVICE_V8_H_

#include "base/basictypes.h"
#include "net/base/net_export.h"

namespace net {

class DhcpProxyScriptFetcher;
class HostResolver;
class NetLog;
class NetworkDelegate;
class ProxyConfigService;
class ProxyScriptFetcher;
class ProxyService;

// Creates a proxy service that polls |proxy_config_service| to notice when
// the proxy settings change. We take ownership of |proxy_config_service|.
//
// |num_pac_threads| specifies the maximum number of threads to use for
// executing PAC scripts. Threads are created lazily on demand.
// If |0| is specified, then a default number of threads will be selected.
//
// Having more threads avoids stalling proxy resolve requests when the
// PAC script takes a while to run. This is particularly a problem when PAC
// scripts do synchronous DNS resolutions, since that can take on the order
// of seconds.
//
// However, the disadvantages of using more than 1 thread are:
//   (a) can cause compatibility issues for scripts that rely on side effects
//       between runs (such scripts should not be common though).
//   (b) increases the memory used by proxy resolving, as each thread will
//       duplicate its own script context.

// |proxy_script_fetcher| specifies the dependency to use for downloading
// any PAC scripts. The resulting ProxyService will take ownership of it.
//
// |dhcp_proxy_script_fetcher| specifies the dependency to use for attempting
// to retrieve the most appropriate PAC script configured in DHCP. The
// resulting ProxyService will take ownership of it.
//
// |host_resolver| points to the host resolving dependency the PAC script
// should use for any DNS queries. It must remain valid throughout the
// lifetime of the ProxyService.
//
// ##########################################################################
// # See the warnings in net/proxy/proxy_resolver_v8.h describing the
// # multi-threading model. In order for this to be safe to use, *ALL* the
// # other V8's running in the process must use v8::Locker.
// ##########################################################################
NET_EXPORT ProxyService* CreateProxyServiceUsingV8ProxyResolver(
    ProxyConfigService* proxy_config_service,
    size_t num_pac_threads,
    ProxyScriptFetcher* proxy_script_fetcher,
    DhcpProxyScriptFetcher* dhcp_proxy_script_fetcher,
    HostResolver* host_resolver,
    NetLog* net_log,
    NetworkDelegate* network_delegate);

}  // namespace net

#endif  // NET_PROXY_PROXY_SERVICE_V8_H_
