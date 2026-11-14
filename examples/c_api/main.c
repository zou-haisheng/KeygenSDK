#include <KeygenSDK/keygensdk_c.h>

#include <stdio.h>

int main(void) {
    KeygenSDK_Config config = {0};
    config.host = "https://example.com";
    config.account_id = "test-account";
    config.timeout_seconds = 15;

    KeygenSDK_Client* client = KeygenSDK_Client_Create(&config);
    if (!client) {
        printf("failed to create client\n");
        return 1;
    }

    printf("has_local_license=%d\n", KeygenSDK_Client_HasLocalLicense(client));
    printf("none=%s\n", KeygenSDK_ErrorCode_ToString(KEYGENSDK_ERROR_NONE));

    KeygenSDK_Client_Destroy(client);
    return 0;
}
