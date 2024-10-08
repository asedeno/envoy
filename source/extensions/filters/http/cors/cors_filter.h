#pragma once

#include "envoy/http/filter.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "source/common/buffer/buffer_impl.h"

#include "absl/container/inlined_vector.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Cors {

/**
 * All CORS filter stats. @see stats_macros.h
 */
#define ALL_CORS_STATS(COUNTER)                                                                    \
  COUNTER(origin_valid)                                                                            \
  COUNTER(origin_invalid)

/**
 * Struct definition for CORS stats. @see stats_macros.h
 */
struct CorsStats {
  ALL_CORS_STATS(GENERATE_COUNTER_STRUCT)
};

/**
 * Configuration for the CORS filter.
 */
class CorsFilterConfig {
public:
  CorsFilterConfig(const std::string& stats_prefix, Stats::Scope& scope);
  CorsStats& stats() { return stats_; }

private:
  static CorsStats generateStats(const std::string& prefix, Stats::Scope& scope) {
    return CorsStats{ALL_CORS_STATS(POOL_COUNTER_PREFIX(scope, prefix))};
  }

  CorsStats stats_;
};
using CorsFilterConfigSharedPtr = std::shared_ptr<CorsFilterConfig>;

class CorsFilter : public Http::StreamFilter {
public:
  CorsFilter(CorsFilterConfigSharedPtr config);

  void initializeCorsPolicies();

  // Http::StreamFilterBase
  void onDestroy() override {}

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers,
                                          bool end_stream) override;
  Http::FilterDataStatus decodeData(Buffer::Instance&, bool) override {
    return Http::FilterDataStatus::Continue;
  };
  Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap&) override {
    return Http::FilterTrailersStatus::Continue;
  };
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override;

  // Http::StreamEncoderFilter
  Http::Filter1xxHeadersStatus encode1xxHeaders(Http::ResponseHeaderMap&) override {
    return Http::Filter1xxHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::ResponseHeaderMap& headers,
                                          bool end_stream) override;
  Http::FilterDataStatus encodeData(Buffer::Instance&, bool) override {
    return Http::FilterDataStatus::Continue;
  };
  Http::FilterTrailersStatus encodeTrailers(Http::ResponseTrailerMap&) override {
    return Http::FilterTrailersStatus::Continue;
  };
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
    return Http::FilterMetadataStatus::Continue;
  }
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override {
    encoder_callbacks_ = &callbacks;
  };

  const auto& policiesForTest() const { return policies_; }

private:
  friend class CorsFilterTest;

  absl::Span<const Matchers::StringMatcherPtr> allowOrigins();
  absl::string_view allowMethods();
  absl::string_view allowHeaders();
  absl::string_view exposeHeaders();
  absl::string_view maxAge();
  bool allowCredentials();
  bool allowPrivateNetworkAccess();
  bool shadowEnabled();
  bool enabled();
  bool isOriginAllowed(const Http::HeaderString& origin);
  bool forwardNotMatchingPreflights();

  Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};
  absl::InlinedVector<std::reference_wrapper<const Envoy::Router::CorsPolicy>, 4> policies_;
  bool is_cors_request_{};
  std::string latched_origin_;

  CorsFilterConfigSharedPtr config_;
};

} // namespace Cors
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
