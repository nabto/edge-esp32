#include "iam_config.h"

#include "json_config.h"

#include <modules/policies/nm_policy.h>
#include <modules/policies/nm_statement.h>
#include <modules/policies/nm_policies_to_json.h>
#include <modules/policies/nm_policies_from_json.h>
#include <modules/iam/nm_iam_role.h>
#include <modules/iam/nm_iam_to_json.h>
#include <modules/iam/nm_iam_from_json.h>

#include <stdio.h>

static const char* LOGM = "iam_config";

void load_iam_config(struct nm_iam* iam)
{
    struct nm_policy* passwordPairingPolicy = nm_policy_new("PasswordPairing");
    {
        struct nm_statement* stmt = nm_statement_new(NM_EFFECT_ALLOW);
        nm_statement_add_action(stmt, "Pairing:Get");
        nm_statement_add_action(stmt, "Pairing:Password");
        nm_statement_add_action(stmt, "Pairing:Local");
        nm_policy_add_statement(passwordPairingPolicy, stmt);
    }

    struct nm_policy* tunnelAllPolicy = nm_policy_new("TunnelAll");
    {
        struct nm_statement* stmt = nm_statement_new(NM_EFFECT_ALLOW);
        nm_statement_add_action(stmt, "TcpTunnel:GetService");
        nm_statement_add_action(stmt, "TcpTunnel:Connect");
        nm_statement_add_action(stmt, "TcpTunnel:ListServices");
        nm_policy_add_statement(tunnelAllPolicy, stmt);
    }

    struct nm_policy* pairedPolicy = nm_policy_new("Paired");
    {
        struct nm_statement* stmt = nm_statement_new(NM_EFFECT_ALLOW);
        nm_statement_add_action(stmt, "Pairing:Get");
        nm_policy_add_statement(pairedPolicy, stmt);
    }

    nm_iam_add_policy(iam, passwordPairingPolicy);
    nm_iam_add_policy(iam, tunnelAllPolicy);
    nm_iam_add_policy(iam, pairedPolicy);

    struct nm_iam_role* unpairedRole = nm_iam_role_new("Unpaired");
    nm_iam_role_add_policy(unpairedRole, "PasswordPairing");

    struct nm_iam_role* adminRole = nm_iam_role_new("Admin");
    nm_iam_role_add_policy(adminRole, "TunnelAll");
    nm_iam_role_add_policy(adminRole, "Paired");

    struct nm_iam_role* userRole = nm_iam_role_new("User");
    nm_iam_role_add_policy(userRole, "TunnelAll");
    nm_iam_role_add_policy(userRole, "Paired");


    nm_iam_add_role(iam, unpairedRole);
    nm_iam_add_role(iam, adminRole);
    nm_iam_add_role(iam, userRole);
}
