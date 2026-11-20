/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_DidResolver_h
#define netwerk_protocol_atproto_DidResolver_h

#include <string>
#include <string_view>

namespace mozilla::net::atproto {

// DID methods raindrop can resolve without a gateway. Unknown methods are
// reported so the caller can fail honestly instead of guessing an endpoint.
enum class DidMethod {
  Unknown,
  Plc,
  Web,
};

const char* DidMethodName(DidMethod aMethod);

struct Did {
  std::string mFull;      // the complete did:<method>:<id> string
  DidMethod mMethod;
  std::string mId;        // method-specific identifier (still percent-encoded)
};

enum class DidError {
  None,
  TooLong,
  InvalidSyntax,
  UnsupportedMethod,
  InvalidIdentifier,
};

const char* DidErrorName(DidError aError);

// Parse and dispatch a DID. Does not perform network I/O.
DidError ParseDid(std::string_view aDid, Did& aResult);

enum class DidUrlError {
  None,
  InvalidDid,
  InvalidWebHost,
  InvalidWebPath,
};

const char* DidUrlErrorName(DidUrlError aError);

// Build the HTTPS URL from which a DID document is fetched:
//   did:plc:...  -> https://plc.directory/<did>
//   did:web:...  -> https://<host>[:port]/<path>/did.json
DidUrlError BuildDidDocumentUrl(std::string_view aDid, std::string& aUrl);

enum class PdsError {
  None,
  TooLarge,
  InvalidJson,
  IdMismatch,
  MissingId,
  MissingService,
  InvalidService,
  InvalidEndpoint,
  OnionEndpoint,
};

const char* PdsErrorName(PdsError aError);

// Maximum DID document size accepted from a resolver endpoint (1 MiB). Larger
// documents are rejected before parsing to bound memory use on untrusted input.
constexpr size_t kMaxDidDocumentLength = 1024 * 1024;

struct PdsService {
  std::string mEndpoint;
};

enum class HandleBindingError {
  None,
  InvalidJson,
  MissingAlsoKnownAs,
  HandleNotClaimed,
};

const char* HandleBindingErrorName(HandleBindingError aError);

// Verify the reverse binding required for handle authorities (ADR 0003): the
// DID document's alsoKnownAs array must contain at://<handle>. A handle that
// resolves to a DID whose document does not claim it is a failed resolution.
HandleBindingError VerifyHandleBinding(std::string_view aJson,
                                       std::string_view aHandle);

// Extract the #atproto_pds service endpoint from a DID document. The document's
// "id" must match aExpectedDid: this is the check that stops a resolver from
// substituting another identity's PDS.
PdsError ExtractPdsEndpoint(std::string_view aJson,
                            std::string_view aExpectedDid,
                            PdsService& aResult);

}  // namespace mozilla::net::atproto

#endif
