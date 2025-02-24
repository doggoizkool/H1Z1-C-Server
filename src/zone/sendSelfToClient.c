u32 GetActorModelId(SessionState* session) {
    u32 headId = session->pGetPlayerActor.headType;

    switch (headId) {
        case 1: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 2: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 3: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 4: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 5: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 6: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 7: {
            session->pGetPlayerActor.actorModelId = 9240;
            return session->pGetPlayerActor.actorModelId;
        } break;
        case 8: {
            session->pGetPlayerActor.actorModelId = 9474;
            return session->pGetPlayerActor.actorModelId;
        } break;
        default: {
            return 0;
        }
    }
}

u32 GetGender(SessionState* session) {
    u32 actorModelId = session->pGetPlayerActor.actorModelId;

    switch (actorModelId) {
        case 9240: {
            session->pGetPlayerActor.gender = 1;
            return session->pGetPlayerActor.gender;
        } break;
        case 9474: {
            session->pGetPlayerActor.gender = 2;
            return session->pGetPlayerActor.gender;
        } break;
        default: {
            return 0;
        }
    }
}

/*
char *GetHeadActor(SessionState *session)
{
}

u32 GetHeadActorLen()
{
}
*/

char* GetHairModel(u32 actorModelId) {
    switch (actorModelId) {
        case 9240:
            printf("Male Hair Model Selected!\n");
            return "SurvivorMale_Hair_MediumMessy.adr";
        case 9474:
            printf("Female Hair Model Selected!\n");
            return "SurvivorFemale_Hair_ShortBun.adr";
        default:
            return "";
    }
}

u32 GetHairModelLen(char* hairModel) {
    if (hairModel) {
        return STRLEN(hairModel);
    } else {
        printf("Hair Model Length IS ZERO!\n");
        return 0;
    }
}

u32 getResourceType(u32 resourceId) {
    switch (resourceId) {
        case HEALTHID:
            return HEALTHTYPE;
        case HUNGERID:
            return HUNGERTYPE;
        case HYDRATIONID:
            return HYDRATIONTYPE;
        case STAMINAID:
            return STAMINATYPE;
        case VIRUSID:
            return VIRUSTYPE;
        case BLEEDINGID:
            return BLEEDINGTYPE;
        case COMFORTID:
            return COMFORTTYPE;
        case FUELID:
            return FUELTYPE;
        case CONDITIONID:
            return CONDITIONTYPE;
        default:
            return 0;
    }
}

void SendSelfToClient(AppState* app, SessionState* session) {
    Zone_Packet_SendSelfToClient sendSelf = { 0 };

    sendSelf.payload_self = (struct payload_self_s[1]){
        [0] = {
            .character_id = session->characterId,
            .guid = session->characterId,
            .transient_id.value = 52,
            .position = {.x = -32.26f, .y = 506.41f, .z = 280.21f, .w = 1.f},
            .rotation = {.x = -0.11f, .y = -0.58f, .z = -0.08f, .w = 1.f},
            .head_id = 1,
            .actor_model_id = 9240,
            .gender1 = 1,
            .head_actor = "SurvivorMale_Head_01.adr", // GetHeadActor(session),
            .head_actor_length = STRLEN("SurvivorMale_Head_01.adr"),
            .hair_model = "SurvivorMale_Hair_MediumMessy.adr", // GetHairModel(session),
            .hair_model_length = STRLEN("SurvivorMale_Hair_MediumMessy.adr"),
            .is_respawning = false,
            .character_name = session->characterName.name,
            .character_name_length = session->characterName.nameLen,
            .loadout_id = 3,
            .current_slot_id = 7,
            .character_resources_count = 1,
            .character_resources = (struct character_resources_s[1]){
                [0] = {
                    .resource_type1 = 1,
                    .resource_id = 1,
                    .resource_type2 = 1,
                    .value = 10000,

                    .unk_array_22866_count = 1,
                    .unk_array_22866 = (struct unk_array_22866_s[1]){
                        [0] = {
                            .unk_dword_129 = 1,
                            .unk_dword_294 = 1,
                            .unk_dword_359 = 1,
                            .unk_dword_430 = 10000,
                        },
                    },
                },
            },
            .is_admin = true,
        },
    };

    ZonePacketSend(app, session, &app->arenaPerTick, KB(50), Zone_Packet_Kind_SendSelfToClient,
                   &sendSelf);

    // ZonePacketRawFileSend(app, session, &app->arenaPerTick, KB(40),
    // "H:\\H1Z1-KotK-C\\H1Z1-C-Server\\src\\zone\\5.bin");
}