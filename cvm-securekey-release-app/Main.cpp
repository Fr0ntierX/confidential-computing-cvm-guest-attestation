//-------------------------------------------------------------------------------------------------
// <copyright file="Main.cpp" company="Microsoft Corporation">
// Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-------------------------------------------------------------------------------------------------

// TODO: Run CodeQL, static analysis on the native code. Also enable it in the repo.

#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <AttestationClient.h>
#include "AttestationUtil.h"
#include "Constants.h"

void usage(char *programName)
{
    printf("Usage: \n");
    printf("\tRelease RSA or EC key:\n");
    printf("\t\t%s -a <attestation-endpoint> -n <optional-nonce> -k KeyURL -c (imds|sp) -r \n", programName);
    printf("\n");
    printf("\tRelease RSA key and wrap/unwrap symmetric key:\n");
    printf("\t\t%s -a <attestation-endpoint> -n <optional-nonce> -k KEYURL -c (imds|sp) -s symkey|base64(wrappedSymKey) -w|-u (Wrap|Unwrap) \n", programName);
    printf("\n");
    printf("\tRelease RSA key or EC key without decrypting released cipher text:\n");
    printf("\t\t%s -a <attestation-endpoint> -n <optional-nonce> -k KeyURL -c (imds|sp) -x -m <optional-maa-token> \n", programName);
    printf("\n");
    printf("\tDecrypt cipher text:\n");
    printf("\t\t%s -d <base64-cipher-text> \n", programName);
    printf("\n");
    printf("\tGet attestation token:\n");
    printf("\t\t%s -a <attestation-endpoint> -n <optional-nonce> -g \n", programName);
}

enum class Operation
{
    None,
    WrapKey,
    UnwrapKey,
    ReleaseKey,
    ExtractCipherText,
    DecryptCipherText,
    GetToken,
    Undefined
};

// Check if tracing is to be enabled for SKR in the env.
void set_tracing(void)
{
    auto skr_trace_flag = std::getenv("SKR_TRACE_ON");
    if(skr_trace_flag != nullptr && strlen(skr_trace_flag) > 0)
    {
        if(strcmp(skr_trace_flag, "1") ==0 || strcmp(skr_trace_flag, "2") ==0)
        {
            std::cout<< "Tracing is enabled" <<std::endl;
            Util::set_trace(true);
            Util::set_trace_level(atoi(skr_trace_flag));
        }
        else
        {
            std::cerr<<"Invalid value for SKR_TRACE_ON!"<<std::endl;
            exit(-1);
        }
    }
}

int main(int argc, char *argv[])
{
    set_tracing();
    TRACE_OUT("Main started");
    std::string attestation_url;
    std::string nonce;
    std::string sym_key;
    std::string key_enc_key_url;
    std::string external_maa_token;
    std::string cipher_text_base64;
    Operation op = Operation::None;
    Util::AkvCredentialSource akv_credential_source = Util::AkvCredentialSource::Imds;

    int opt;
    while ((opt = getopt(argc, argv, "a:n:k:c:s:uwrm:xd:g")) != -1)
    {
        switch (opt)
        {
        case 'a':
            attestation_url.assign(optarg);
            TRACE_OUT("attestation_url: %s", attestation_url.c_str());
            break;
        case 'n':
            nonce.assign(optarg);
            TRACE_OUT("nonce: %s", nonce.c_str());
            break;
        case 'k':
            key_enc_key_url.assign(optarg);
            TRACE_OUT("key_enc_key_url: %s", key_enc_key_url.c_str());
            break;
        case 'c':
            if (strcmp(optarg, "imds") == 0)
            {
                akv_credential_source = Util::AkvCredentialSource::Imds;
            }
            else if (strcmp(optarg, "sp") == 0)
            {
                akv_credential_source = Util::AkvCredentialSource::EnvServicePrincipal;
            }
            TRACE_OUT("akv_credential_source: %d", static_cast<int>(akv_credential_source));
            break;
        case 'u':
            op = Operation::UnwrapKey;
            TRACE_OUT("op: %d", static_cast<int>(op));
            break;
        case 'w':
            op = Operation::WrapKey;
            TRACE_OUT("op: %d", static_cast<int>(op));
            break;
        case 's':
            sym_key.assign(optarg);
            TRACE_OUT("sym_key: %s", sym_key.c_str());
            break;
        case 'm':
            external_maa_token.assign(optarg);
            TRACE_OUT("external_maa_token: %s", external_maa_token.c_str());
            break;
        case 'r':
            op = Operation::ReleaseKey;
            TRACE_OUT("op: %d", static_cast<int>(op));
            break;
        case 'x':
            op = Operation::ExtractCipherText;
            TRACE_OUT("op: %d", static_cast<int>(op));
            break;
        case 'd':
            cipher_text_base64.assign(optarg);
            op = Operation::DecryptCipherText;
            TRACE_OUT("cipher_text_base64: %s", cipher_text_base64.c_str());
            TRACE_OUT("op: %d", static_cast<int>(op));
            break;
        case 'g':
            op = Operation::GetToken;
            TRACE_OUT("op: %d", static_cast<int>(op));
            break;
        case ':':
            std::cerr << "Option needs a value" << std::endl;
            return EXIT_FAILURE;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    bool success = false;
    int retVal = 0;
    try
    {
        std::string result;
        switch (op)
        {
        case Operation::WrapKey:
            result = Util::WrapKey(attestation_url, nonce, sym_key, key_enc_key_url, akv_credential_source, external_maa_token);
            std::cout << result << std::endl;
            break;
        case Operation::UnwrapKey:
            result = Util::UnwrapKey(attestation_url, nonce, sym_key, key_enc_key_url, akv_credential_source, external_maa_token);
            std::cout << result << std::endl;
            break;
        case Operation::ReleaseKey:
            success = Util::ReleaseKey(attestation_url, nonce, key_enc_key_url, akv_credential_source, external_maa_token);
            retVal = success ? EXIT_SUCCESS : EXIT_FAILURE;
            break;
        case Operation::ExtractCipherText:
            result = Util::ExtractCipherText(attestation_url, nonce, key_enc_key_url, akv_credential_source, external_maa_token);
            std::cout << result << std::endl;
            break;
        case Operation::DecryptCipherText:
            result = Util::DecryptCipherText(cipher_text_base64);
            std::cout << result << std::endl;
            break;
        case Operation::GetToken:
            result = Util::GetMAAToken(attestation_url, nonce);
            std::cout << result << std::endl;
            break;
        default:
            usage(argv[0]);
            retVal = EXIT_FAILURE;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception occured. Details: " << e.what() << std::endl;
        retVal = EXIT_FAILURE;
    }

    return retVal;
}
