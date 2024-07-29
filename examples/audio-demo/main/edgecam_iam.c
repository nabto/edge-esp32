#include <apps/common/random_string.h>


#include "edgecam_iam.h"

#include <modules/iam/nm_iam_serializer.h>

#include <cjson/cJSON.h>

#define TAG "tcptunnel_iam"

struct nm_iam_state* tcptunnel_create_default_iam_state(NabtoDevice* device)
{
    struct nm_iam_state* state = nm_iam_state_new();
    nm_iam_state_set_open_pairing_role(state, "Administrator");
    nm_iam_state_set_local_initial_pairing(state, false);
    nm_iam_state_set_local_open_pairing(state, true);
    nm_iam_state_set_password_open_password(state, "pairpsw");
    printf("nu har jeg sat password\n");
    nm_iam_state_set_password_open_pairing(state, true);
    char* sct = NULL;
    nabto_device_create_server_connect_token(device, &sct);
    nm_iam_state_set_password_open_sct(state, sct);
    nabto_device_string_free(sct);
    return state;
}

struct nm_iam_configuration* tcptunnel_create_iam_config()
{
    struct nm_iam_configuration* conf = nm_iam_configuration_new();
    {
        // Policy allowing pairing
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("Pairing");
        struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
        nm_iam_configuration_statement_add_action(s, "IAM:GetPairing");
        nm_iam_configuration_statement_add_action(s, "IAM:PairingPasswordOpen");
        nm_iam_configuration_statement_add_action(s, "IAM:PairingPasswordInvite");
        nm_iam_configuration_statement_add_action(s, "IAM:PairingLocalInitial");
        nm_iam_configuration_statement_add_action(s, "IAM:PairingLocalOpen");
        nm_iam_configuration_add_policy(conf, p);
    }
    {
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("Tunnelling");
        struct nm_iam_statement* stmt = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
        nm_iam_configuration_statement_add_action(stmt, "TcpTunnel:GetService");
        nm_iam_configuration_statement_add_action(stmt, "TcpTunnel:Connect");
        nm_iam_configuration_statement_add_action(stmt, "TcpTunnel:ListServices");
        nm_iam_configuration_add_policy(conf, p);
    }
    {
        // Policy allowing management of IAM state
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("ManageIam");
        struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
        nm_iam_configuration_statement_add_action(s, "IAM:ListUsers");
        nm_iam_configuration_statement_add_action(s, "IAM:GetUser");
        nm_iam_configuration_statement_add_action(s, "IAM:DeleteUser");
        nm_iam_configuration_statement_add_action(s, "IAM:SetUserRole");
        nm_iam_configuration_statement_add_action(s, "IAM:ListRoles");
        nm_iam_configuration_statement_add_action(s, "IAM:SetSettings");
        nm_iam_configuration_statement_add_action(s, "IAM:GetSettings");

        // not from esp-eye demo
        /*
        nm_iam_configuration_statement_add_action(s, "IAM:CreateUser");
        nm_iam_configuration_statement_add_action(s, "IAM:SetUserPassword");
        nm_iam_configuration_statement_add_action(s, "IAM:SetUserDisplayName");
        nm_iam_configuration_statement_add_action(s, "IAM:SetDeviceInfo");
        */
        
        nm_iam_configuration_add_policy(conf, p);
    }

    {
        // Policy allowing management of own IAM user
        struct nm_iam_policy* p = nm_iam_configuration_policy_new("ManageOwnUser");
        {
            struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
            nm_iam_configuration_statement_add_action(s, "IAM:GetUser");
            nm_iam_configuration_statement_add_action(s, "IAM:DeleteUser");
            nm_iam_configuration_statement_add_action(s, "IAM:SetUserDisplayName");

            // Create a condition such that only connections where the
            // UserId matches the UserId of the operation is allowed. E.g. IAM:Username == ${Connection:Username}

            struct nm_iam_condition* c = nm_iam_configuration_statement_create_condition(s, NM_IAM_CONDITION_OPERATOR_STRING_EQUALS, "IAM:Username");
            nm_iam_configuration_condition_add_value(c, "${Connection:Username}");
        }
        {
            struct nm_iam_statement* s = nm_iam_configuration_policy_create_statement(p, NM_IAM_EFFECT_ALLOW);
            nm_iam_configuration_statement_add_action(s, "IAM:ListUsers");
            nm_iam_configuration_statement_add_action(s, "IAM:ListRoles");
        }

        nm_iam_configuration_add_policy(conf, p);
    }

    {
        // Role allowing unpaired connections to pair
        struct nm_iam_role* r = nm_iam_configuration_role_new("Unpaired");
        nm_iam_configuration_role_add_policy(r, "Pairing");
        nm_iam_configuration_add_role(conf,r);
    }
    {
        // Role allowing everything assigned to administrators
        struct nm_iam_role* r = nm_iam_configuration_role_new("Administrator");
        nm_iam_configuration_role_add_policy(r, "ManageOwnUser");
        nm_iam_configuration_role_add_policy(r, "ManageIam");
        nm_iam_configuration_role_add_policy(r, "Pairing");
        nm_iam_configuration_role_add_policy(r, "Tunnelling");
        nm_iam_configuration_add_role(conf, r);
    }

    // Connections which does not have a paired user in the system gets the Unpaired role.
    nm_iam_configuration_set_unpaired_role(conf, "Unpaired");
    return conf;
}


char* tcptunnel_iam_create_pairing_string(struct nm_iam* iam, const char* productId, const char* deviceId)
{
    struct nm_iam_state* state = nm_iam_dump_state(iam);
    if (state == NULL) {
        return NULL;
    }

    char* pairStr = (char*)calloc(1, 512);
    if (pairStr == NULL) {
        nm_iam_state_free(state);
        return NULL;
    }

    snprintf(pairStr, 511, "p=%s,d=%s,pwd=%s,sct=%s", productId, deviceId,
             state->passwordOpenPassword, state->passwordOpenSct);

    nm_iam_state_free(state);
    return pairStr;
}
