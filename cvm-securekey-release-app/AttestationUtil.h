//-------------------------------------------------------------------------------------------------
// <copyright file="AttestationUtil.h" company="Microsoft Corporation">
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-------------------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stdarg.h>

#ifdef _MSC_VER
#include <windows.h> // for HRESULT
#include <bcrypt.h>
// #include <fveapi.h>
#else
typedef int HRESULT;
#define FAILED(hr) (((HRESULT)(hr)) < 0)
typedef unsigned long DWORD;
#define GetLastError() errno
#endif

#include <openssl/opensslv.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pkcs7.h>
#include <openssl/pem.h>

typedef unsigned char BYTE;
typedef unsigned char *PBYTE;

#define CHECK_HR(hr)                                     \
    do                                                   \
    {                                                    \
        Check_HResult(__FILE__, __func__, __LINE__, hr); \
    } while (0);

inline static void Check_HResult(std::string fileName, std::string funcName, int lineNo, HRESULT hr)
{
    if (FAILED(hr))
    {
        DWORD gle = GetLastError();
        fprintf(stderr, "Failed in %s:%s() on line %d. HR=0x%0x", fileName.c_str(), funcName.c_str(), lineNo, hr);
        exit(gle);
    }
}

#define TRACE_ERROR_EXIT(msg)                                                                            \
    do                                                                                                   \
    {                                                                                                    \
        DWORD gle = GetLastError();                                                                      \
        fprintf(stderr, "Error occured in %s:%s on line %d. Msg=%s", __FILE__, __func__, __LINE__, msg); \
        exit(gle);                                                                                       \
    } while (0);

#define TRACE_OUT Util::trace_out
#define OSSL_BN_TRACE_OUT Util::ossl_bn_trace_out

class Util
{
private:
    static bool isTraceOn;
    static int traceLevel; // 1: enable Util::reduct_log, 2: do nothing.
    static size_t lengthMask;

public:
    static void set_trace(bool traceOn)
    {
        isTraceOn = traceOn;
    }

    static bool get_trace()
    {
        return isTraceOn;
    }

    static void set_trace_level(int trLevel)
    {
        traceLevel = trLevel;
    }

    static int get_trace_level()
    {
        return traceLevel;
    }

    inline static void trace_out(std::string fmt, ...)
    {
        if (isTraceOn)
        {
            va_list args;
            va_start(args, fmt);
            vfprintf(stderr, fmt.c_str(), args);
            fprintf(stderr, "\n");
            va_end(args);
        }
    }

    inline static std::string reduct_log(const std::string &str)
    {
        std::string retStr(str);
        if (traceLevel == 1)
        {
            double percentage = reduct_log_percentage(str);
            size_t lengthMask = retStr.size() * (percentage / 100);
            if (retStr.size() > lengthMask)
            {
                retStr.resize(lengthMask);
                retStr.append("...");
            }
        }
        return retStr.c_str();
    }

    inline static double reduct_log_percentage(const std::string &str)
    {
        std::string err = "error";
        auto it = std::search(str.begin(), str.end(), err.begin(), err.end(),
                              [](char a, char b)
                              { return std::tolower(a) == std::tolower(b); });
        if (it != str.end())
            return 100;
        return 15;
    }

    inline static void ossl_bn_trace_out(const BIGNUM *bn)
    {
        if (isTraceOn)
        {
            BN_print_fp(stderr, bn);
        }
    }

    enum class AkvCredentialSource
    {
        Imds,
        EnvServicePrincipal
    };

    /// <summary>
    /// Convert a base64 encoded string to a vector of bytes.
    /// </summary>
    /// <param name="base64_data">Base64 encoded string</param>
    /// <returns>Vector of BYTES</returns>
    static std::vector<BYTE> base64_to_binary(const std::string &base64_data);

    /// <summary>
    /// Convert a vector of bytes to a base64 encoded string.
    /// </summary>
    /// <param name="binary_data">Vector of unsinged chars.</param>
    /// <returns>Base64 encoded string</returns>
    static std::string binary_to_base64(const std::vector<BYTE> &binary_data);

    /// <summary>
    /// Convert a vector of bytes to a base64url encoded string.
    /// </summary>
    /// <param name="binary_data">Vector of BYTES.</param>
    /// <returns>Base64url encoded string.</returns>
    static std::string binary_to_base64url(const std::vector<BYTE> &binary_data);

    /// <summary>
    /// Convert a vector of bytes to a hex encoded string.
    /// </summary>
    /// <param name="binary_data">Vector of BYTES.</param>
    /// <returns>Hex encoded string.</returns>
    static std::string binary_to_hex(const std::vector<BYTE> &binary_data);

    /// <summary>
    /// Convert a hex encoded string to vector of bytes.
    /// </summary>
    /// <param name="hex_data">Hex encoded data.</param>
    /// <returns>Vector of BYTES.</returns>
    static std::vector<BYTE> hex_to_binary(const std::string &hex_data);

    /// <summary>
    /// Convert a base64url encoded string to a vector of bytes.
    /// </summary>
    /// <param name="base64url_data">Base64url encoded string.</param>
    /// <returns>Vector of BYTES.</returns>
    static std::vector<BYTE> base64url_to_binary(const std::string &base64url_data);

    /// <summary>
    /// Convert a base64 decoded string to string.
    /// </summary>
    /// <param name="data">Base64 encoded string.</param>
    /// <returns>Plain string</returns>
    static std::string base64_decode(const std::string &data);

    /// <summary>
    /// Encode input string for url
    /// </summary>
    /// <param name="data">input data</param>
    /// <returns>Escaped string</returns>
    static std::string url_encode(const std::string &data);

    /// <summary>
    /// Callback for curl perform operation.
    /// </summary>
    static size_t CurlWriteCallback(char *data, size_t size, size_t nmemb, std::string *buffer);

    /// <summary>
    /// Get a REST URL for secure key release
    /// </summary>
    /// <param name="KEKUrl">Key encryption key URL</param>
    /// <returns>REST URL string</returns>
    static std::string GetKeyVaultSKRurl(const std::string &KEKUrl);

    /// <summary>
    /// Get secure key release response from KMS (Key Vault or mHSM)
    /// </summary>
    /// <param name="requestUri">SKR request URL</param>
    /// <param name="access_token">Authorization token</param>
    /// <param name="attestation_token">Attestation token</param>
    /// <param name="nonce">Nonce value to send</param>
    /// <returns>SKR reponse from KMS</returns>
    static std::string GetKeyVaultResponse(const std::string &requestUri,
                                           const std::string &access_token,
                                           const std::string &attestation_token,
                                           const std::string &nonce);

    /// <summary>
    /// Retrieve MSI token from IMDS servce
    /// </summary>
    /// <param name="KEKUrl">Key encryption key URL</param>
    /// <returns>MSI token for the resource</returns>
    static std::string GetIMDSToken(const std::string &KEKUrl);

    // <summary>
    /// Retrieve MSI token from Service Principal Credentials available in the Environment Variables
    /// </summary>
    /// <param name="KEKUrl">Key encryption key URL</param>
    /// <returns>MSI token for the resource</returns>
    static std::string GetAADToken(const std::string &KEKUrl);

    /// <summary>
    /// Get attestation token from the attestation service.
    /// </summary>
    /// <param name="attestation_url">Attestation service URL.</param>
    /// <param name="nonce">unique nonce per attestation request.</param>
    /// <returns>MAA token</returns>
    static std::string GetMAAToken(const std::string &attestation_url, const std::string &nonce);

    /// <summary>
    /// Split string by delimeter.
    /// </summary>
    /// <param name="delimiter">Delimeter character.</param>
    /// <returns>Vector of strings</returns>
    static std::vector<std::string> SplitString(const std::string &s, char delimiter);

    /// <summary>
    /// Do secure key release (SKR) to get the key encryption key (KEK).
    /// </summary>
    /// <param name="attestation_url">Attestation service URL.</param>
    /// <param name="nonce">unique nonce per attestation request.</param>
    /// <param name="KEKUrl">Key encryption key URL</param>
    /// <param name="pkey">OpenSSL key representation</param>
    /// <param name="akv_credential_source">AkvCredentialSource type for accessing Key Vault</param>
    /// <returns>True if successful</returns>
    static bool doSKR(const std::string &attestation_url,
                      const std::string &nonce,
                      std::string KEKUrl,
                      EVP_PKEY **pkey,
                      const Util::AkvCredentialSource &akv_credential_source,
                      const std::string &external_maa_token = ""
                    );

    /// <summary>
    /// Wrap the symmetric key with the public key of the key encryption key (KEK).
    /// </summary>
    /// <param name="attestation_url">Attestation service URL.</param>
    /// <param name="nonce">unique nonce per attestation request.</param>
    /// <param name="plainText">Plain text symmetric key</param>
    /// <param name="key_enc_key">KEK</param>
    /// <param name="akv_credential_source">AkvCredentialSource type for accessing Key Vault</param>
    /// <returns>Wrapped key</returns>
    static std::string WrapKey(const std::string &attestation_url,
                               const std::string &nonce,
                               const std::string &plainText,
                               const std::string &key_enc_key,
                               const Util::AkvCredentialSource &akv_credential_source,
                               const std::string &external_maa_token = ""
                            );

    /// <summary>
    /// Unwrap the symmetric key using the private key of the key encryption key (KEK).
    /// </summary>
    /// <param name="attestation_url">Attestation service URL.</param>
    /// <param name="nonce">unique nonce per attestation request.</param>
    /// <param name="cipherText">Wrapped symmetric key</param>
    /// <param name="key_enc_key">KEK</param>
    /// <param name="akv_credential_source">AkvCredentialSource type for accessing Key Vault</param>
    /// <returns>Plain text symmetric key</returns>
    static std::string UnwrapKey(const std::string &attestation_url,
                                 const std::string &nonce,
                                 const std::string &cipherText,
                                 const std::string &key_enc_key,
                                 const Util::AkvCredentialSource &akv_credential_source,
                                 const std::string &external_maa_token = "" 
                                );

    /// <summary>
    /// Release the RSA or EC private key from KMS.
    /// </summary>
    /// <param name="attestation_url">Attestation service URL.</param>
    /// <param name="nonce">unique nonce per attestation request.</param>
    /// <param name="key_enc_key">KEK</param>
    /// <param name="akv_credential_source">AkvCredentialSource type for accessing Key Vault</param>
    /// <returns>True if key release succeeds, False otherwise</returns>
    static bool ReleaseKey(const std::string &attestation_url,
                           const std::string &nonce,
                           const std::string &key_enc_key,
                           const Util::AkvCredentialSource &akv_credential_source,
                           const std::string &external_maa_token = ""
                        );

    /// <summary>
    /// Extract encrypted cipher text from Key Vault without decrypting (for VM transfer).
    /// </summary>
    /// <param name="attestation_url">Attestation service URL.</param>
    /// <param name="nonce">unique nonce per attestation request.</param>
    /// <param name="key_enc_key_url">Key encryption key URL</param>
    /// <param name="akv_credential_source">AkvCredentialSource type for accessing Key Vault</param>
    /// <param name="external_maa_token">Optional external MAA token to use</param>
    /// <returns>True if successful, False otherwise</returns>
    static bool ExtractCipherText(const std::string &attestation_url,
                                  const std::string &nonce,
                                  const std::string &key_enc_key_url,
                                  const Util::AkvCredentialSource &akv_credential_source,
                                  const std::string &external_maa_token = ""
                                );

    /// <summary>
    /// Decrypt cipher text from base64 string using TPM (VM 2 operation).
    /// </summary>
    /// <param name="cipherTextBase64">Base64-encoded cipher text from VM 1</param>
    /// <returns>True if successful, False otherwise</returns>
    static bool DecryptCipherText(const std::string &cipherTextBase64);
};
