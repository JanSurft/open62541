/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "open62541/types_generated_encoding_binary.h"

#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>

#define UA_AES128CTR_SIGNING_KEY_LENGTH 16
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4


UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

static void
setup(void) {}

static void
teardown(void) {}

static UA_Byte *
hexstr_to_char(const char *hexstr) {
    size_t len = strlen(hexstr);
    if(len % 2 != 0)
        return NULL;
    size_t final_len = len / 2;
    UA_Byte *chrs = (UA_Byte *)malloc((final_len + 1) * sizeof(*chrs));
    for(size_t i = 0, j = 0; j < final_len; i += 2, j++)
        chrs[j] =
            (UA_Byte)((hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i + 1] % 32 + 9) % 25);
    chrs[final_len] = '\0';
    return chrs;
}
START_TEST(SingleSubscribedDataSetField) {

    UA_ByteString buffer;
    buffer.length = 85;
    buffer.data =
        hexstr_to_char("f111ba08016400014df4030100000008b02d012e01000000da434ce02ee19922c"
                       "6e916c8154123baa25f67288e3378d613f32039096e08a9ff14b83ea2247792ee"
                       "ffc757c85ac99c0ffa79e4fbe5629783dc77b403");
    UA_Logger logger;
    UA_NetworkMessage msg;
    memset(&msg, 0, sizeof(UA_NetworkMessage));

    UA_PubSubSecurityPolicy *policy =
        (UA_PubSubSecurityPolicy *)UA_malloc(sizeof(UA_PubSubSecurityPolicy));
    UA_PubSubSecurityPolicy_Aes128Ctr(policy, &logger);
    UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
    UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
    UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

    void **channelContext = (void **)calloc(1, 56);

    policy->newContext(policy->policyContext, &sk, &ek, &kn, channelContext);

    UA_MessageSecurityMode securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    size_t currentPosition = 0;
    readNetworkMessage(
        &logger, securityMode, &buffer, &currentPosition, &msg, channelContext,
        policy->setMessageNonce,
        policy->symmetricModule.cryptoModule.signatureAlgorithm.getLocalSignatureSize,
        policy->symmetricModule.cryptoModule.signatureAlgorithm.verify,
        policy->symmetricModule.cryptoModule.encryptionAlgorithm.decrypt);
}
END_TEST

int
main(void) {
    TCase *tc_pubsub_publish = tcase_create("PubSub publish DataSetFields");
    tcase_add_checked_fixture(tc_pubsub_publish, setup, teardown);
    tcase_add_test(tc_pubsub_publish, SingleSubscribedDataSetField);

    Suite *s = suite_create("PubSub WriterGroups/Writer/Fields handling and publishing");
    suite_add_tcase(s, tc_pubsub_publish);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
