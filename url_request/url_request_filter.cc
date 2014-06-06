// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/url_request_filter.h"

#include <set>

#include "base/logging.h"
#include "base/stl_util.h"
#include "net/url_request/url_request_interceptor.h"

namespace net {

namespace {

class URLRequestFilterInterceptor : public URLRequestInterceptor {
 public:
  explicit URLRequestFilterInterceptor(URLRequest::ProtocolFactory* factory)
      : factory_(factory) {}
  virtual ~URLRequestFilterInterceptor() {}

  // URLRequestInterceptor implementation.
  virtual URLRequestJob* MaybeInterceptRequest(
      URLRequest* request, NetworkDelegate* network_delegate) const OVERRIDE {
    return factory_(request, network_delegate, request->url().scheme());
  }

 private:
  URLRequest::ProtocolFactory* factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFilterInterceptor);
};

}  // namespace

URLRequestFilter* URLRequestFilter::shared_instance_ = NULL;

URLRequestFilter::~URLRequestFilter() {}

// static
URLRequestJob* URLRequestFilter::Factory(URLRequest* request,
                                         NetworkDelegate* network_delegate,
                                         const std::string& scheme) {
  // Returning null here just means that the built-in handler will be used.
  return GetInstance()->MaybeInterceptRequest(request, network_delegate,
                                              scheme);
}

// static
URLRequestFilter* URLRequestFilter::GetInstance() {
  if (!shared_instance_)
    shared_instance_ = new URLRequestFilter;
  return shared_instance_;
}

void URLRequestFilter::AddHostnameHandler(const std::string& scheme,
    const std::string& hostname, URLRequest::ProtocolFactory* factory) {
  AddHostnameInterceptor(
      scheme, hostname,
      scoped_ptr<URLRequestInterceptor>(
          new URLRequestFilterInterceptor(factory)));
}

void URLRequestFilter::AddHostnameInterceptor(
    const std::string& scheme,
    const std::string& hostname,
    scoped_ptr<URLRequestInterceptor> interceptor) {
  DCHECK_EQ(0u, hostname_interceptor_map_.count(make_pair(scheme, hostname)));
  hostname_interceptor_map_[make_pair(scheme, hostname)] =
      interceptor.release();

  // Register with the ProtocolFactory.
  URLRequest::Deprecated::RegisterProtocolFactory(
      scheme, &URLRequestFilter::Factory);

#ifndef NDEBUG
  // Check to see if we're masking URLs in the url_interceptor_map_.
  for (URLInterceptorMap::const_iterator it = url_interceptor_map_.begin();
       it != url_interceptor_map_.end(); ++it) {
    const GURL& url = GURL(it->first);
    HostnameInterceptorMap::const_iterator host_it =
        hostname_interceptor_map_.find(make_pair(url.scheme(), url.host()));
    if (host_it != hostname_interceptor_map_.end())
      NOTREACHED();
  }
#endif  // !NDEBUG
}

void URLRequestFilter::RemoveHostnameHandler(const std::string& scheme,
                                             const std::string& hostname) {
  HostnameInterceptorMap::iterator it =
      hostname_interceptor_map_.find(make_pair(scheme, hostname));
  DCHECK(it != hostname_interceptor_map_.end());

  delete it->second;
  hostname_interceptor_map_.erase(it);
  // Note that we don't unregister from the URLRequest ProtocolFactory as
  // this would leave no protocol factory for the remaining hostname and URL
  // handlers.
}

bool URLRequestFilter::AddUrlHandler(
    const GURL& url,
    URLRequest::ProtocolFactory* factory) {
  return AddUrlInterceptor(
      url,
      scoped_ptr<URLRequestInterceptor>(
          new URLRequestFilterInterceptor(factory)));
}

bool URLRequestFilter::AddUrlInterceptor(
    const GURL& url,
    scoped_ptr<URLRequestInterceptor> interceptor) {
  if (!url.is_valid())
    return false;
  DCHECK_EQ(0u, url_interceptor_map_.count(url.spec()));
  url_interceptor_map_[url.spec()] = interceptor.release();

  // Register with the ProtocolFactory.
  URLRequest::Deprecated::RegisterProtocolFactory(url.scheme(),
                                                  &URLRequestFilter::Factory);
  // Check to see if this URL is masked by a hostname handler.
  DCHECK_EQ(0u, hostname_interceptor_map_.count(make_pair(url.scheme(),
                                                          url.host())));

  return true;
}

void URLRequestFilter::RemoveUrlHandler(const GURL& url) {
  URLInterceptorMap::iterator it = url_interceptor_map_.find(url.spec());
  DCHECK(it != url_interceptor_map_.end());

  delete it->second;
  url_interceptor_map_.erase(it);
  // Note that we don't unregister from the URLRequest ProtocolFactory as
  // this would leave no protocol factory for the remaining hostname and URL
  // handlers.
}

void URLRequestFilter::ClearHandlers() {
  // Unregister with the ProtocolFactory.
  std::set<std::string> schemes;
  for (URLInterceptorMap::const_iterator it= url_interceptor_map_.begin();
       it != url_interceptor_map_.end(); ++it) {
    schemes.insert(GURL(it->first).scheme());
  }
  for (HostnameInterceptorMap::const_iterator it =
           hostname_interceptor_map_.begin();
       it != hostname_interceptor_map_.end(); ++it) {
    schemes.insert(it->first.first);
  }
  for (std::set<std::string>::const_iterator scheme = schemes.begin();
       scheme != schemes.end(); ++scheme) {
    URLRequest::Deprecated::RegisterProtocolFactory(*scheme, NULL);
  }

  STLDeleteValues(&url_interceptor_map_);
  STLDeleteValues(&hostname_interceptor_map_);
  hit_count_ = 0;
}

URLRequestFilter::URLRequestFilter() : hit_count_(0) { }

URLRequestJob* URLRequestFilter::MaybeInterceptRequest(
    URLRequest* request,
    NetworkDelegate* network_delegate,
    const std::string& scheme) {
  URLRequestJob* job = NULL;
  if (request->url().is_valid()) {
    // Check the hostname map first.
    const std::string& hostname = request->url().host();

    HostnameInterceptorMap::iterator i =
        hostname_interceptor_map_.find(make_pair(scheme, hostname));
    if (i != hostname_interceptor_map_.end())
      job = i->second->MaybeInterceptRequest(request, network_delegate);

    if (!job) {
      // Not in the hostname map, check the url map.
      const std::string& url = request->url().spec();
      URLInterceptorMap::iterator i = url_interceptor_map_.find(url);
      if (i != url_interceptor_map_.end())
        job = i->second->MaybeInterceptRequest(request, network_delegate);
    }
  }
  if (job) {
    DVLOG(1) << "URLRequestFilter hit for " << request->url().spec();
    hit_count_++;
  }
  return job;
}

}  // namespace net
