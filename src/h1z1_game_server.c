#if defined(YOTE_INTERNAL)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#else
static void platform_win_console_write(char *format, ...);
#define printf(s, ...) platform_win_console_write(s, __VA_ARGS__)
#endif // YOTE_INTERNAL

#define YOTE_USE_ARENA 1
#define YOTE_USE_STRING 1
#include "yote.h"
#define YOTE_PLATFORM_USE_SOCKETS 1
#include "yote_platform.h"
#include "game_server.h"

#define LOCAL_PORT 60000
#define MAX_FRAGMENTS 12000
#define MAX_PACKET_LENGTH 512
#define DATA_HEADER_LENGTH 4
#define MAX_SESSIONS_COUNT 4

#include "utility/endian.c"
#include "utility/util.c"
#include "utility/crypt_rc4.c"
#include "utility/character_id_gen.c"
#include "utility/transient_id_gen.c"
#include "shared/protocol/stream.h"
#include "shared/protocol/fragment_pool.c"
#include "shared/protocol/input_stream.c"
#include "shared/protocol/output_stream.c"
#include "shared/protocol/core_protocol.h"
#include "shared/connection.h"
#include "shared/session.h"
#include "shared/packet_queue.h"
#include "shared/packet_queue.c"

// thirdparty utility headers go here
#include "thirdparty_utility/jansson.h"

global u64 global_packet_dump_count;
// HACK(rhett):
global u64 global_tick_count;

typedef struct Stream_Function_Table Stream_Function_Table;
struct Stream_Function_Table
{
	input_stream_callback_ack *game_input_ack;
	input_stream_callback_data *game_input_data;
	output_stream_callback_data *game_output_data;
	input_stream_callback_data *ping_input_data;
};

struct App_State
{
	Stream_Function_Table *stream_function_table;
	f32 *tick_ms;
	f32 *work_ms;
	u64 *tick_count;
	Key_States *key_states;

#if defined(TERMINAL_UI)
	Buffer screen;
#endif // TERMINAL_UI

	Arena arena_total;
	Platform_Api *platform_api;
	Arena arena_per_tick;

	Platform_Socket socket;

	u8 rc4_key_decoded[256];
	i32 rc4_key_decoded_length;
	Connection_Args connection_args;

	i32 sessions_capacity;
	Session_State sessions[MAX_SESSIONS_COUNT];
};

internal void gateway_on_login(App_State *app_state, Session_State *session_state, u64 character_id);
internal void gateway_on_tunnel_data_from_client(App_State *app_state, Session_State *session_state, u8 *data, u32 data_length);
internal INPUT_STREAM_CALLBACK_DATA(on_ping_input_stream_data);

#undef MESSAGE_NAMESPACE
#define MESSAGE_NAMESPACE "Core"
#include "shared/protocol/core_protocol.c"
#undef MESSAGE_NAMESPACE
#define MESSAGE_NAMESPACE "Gateway"
#include "game/external_gateway_api_3.c"
#undef MESSAGE_NAMESPACE
// TODO(rhett): Client or Zone? Client is the word used by the game, but zone is more clear?
#define MESSAGE_NAMESPACE "Zone"
#include "../schema/output/client_protocol_1087.c"
#include "../schema/output/login_udp_11.c"
#include "game/client_protocol_1087.c"
#undef MESSAGE_NAMESPACE
#define MESSAGE_NAMESPACE MESSAGE_NAMESPACE_DEFAULT

// TODO(rhett): hardcoded in gateway protocol for now
internal void gateway_on_login(App_State *app_state, Session_State *session_state, u64 character_id)
{
	printf("[!] Character %llxh trying to login to zone server\n", character_id);

	generate_guid(session_state->guid);
	generate_guid(session_state->item_guid);
	generate_guid(session_state->character_id);
	generate_transient_id(session_state->transient_id.value);

	u64 guid_get = 0;
	u64 item_guid_get = 0;
	u64 character_id_get = 0;
	u32 transient_id_get = 0;

	guid_get = get_guid(session_state->guid);
	item_guid_get = get_guid(session_state->item_guid);
	character_id_get = get_guid(session_state->character_id);
	transient_id_get = get_transient_id(session_state->transient_id.value);

	Zone_Packet_InitializationParameters init_params =
		{
			.environment_length = 9,
			.environment = "LIVE_KOTK",
		};

	Zone_Packet_SendZoneDetails send_zone_details =
		{
			.zone_name_length = 9,
			.zone_name = "LoginZone",
			.zone_type = 4,
			.unk_bool = FALSE,

			// set skydata
			.unknownDword1 = 1.0f,
			.fog_density = 0.0001733333f,
			.fog_floor = 10.0f,
			.fog_gradient = 0.0144f,
			.rain = 1.0f,
			.temp = 75.0f,
			.color_gradient = 0.0f,
			.unknown_dword8 = 0.05f,
			.unknown_dword9 = 0.0f,
			.unknown_dword10 = 0.05f,
			.unknown_dword11 = 0.15f,
			.unknown_dword12 = 0.0f,
			.sun_axis_x = 38.0f,
			.sun_axis_y = -15.0f,
			.unknown_dword15 = 0.0f,
			.disable_trees = -1.0f,
			.disable_trees1 = -0.05f,
			.disable_trees2 = -1.0f,
			.wind = 3.0f,
			.unknown_dword20 = 0.0f,
			.unknown_dword21 = 1.0f,

			.name_length = 16,
			.name = "sky_Z_clouds.dds",

			.unknown_dword22 = 0.3f,
			.unknown_dword23 = -0.002f,
			.unknown_dword24 = 0.0f,
			.unknown_dword25 = 1000.0f,
			.unknown_dword26 = 0.2f,
			.unknown_dword27 = 0.0f,
			.unknown_dword28 = 0.002f,
			.unknown_dword29 = 8000.0f,
			.ao_size = 0.0f,
			.ao_gamma = 0.25f,
			.ao_blackpoint = 7.0f,
			.unknown_dword33 = 0.5f,

			.zone_id = 5,
			.zone_id_2 = 5,
			.name_id = 7699,
			.unk_bool2 = TRUE,
			.lighting_length = 15,
			.lighting = "Lighting_Z2.txt",
			.unk_bool3 = FALSE,
			.unk_bool4 = FALSE,
		};

	Zone_Packet_ReferenceDataWeaponDefinitions weapon_defs =
		{
			.weapon_byteswithlength =
				(struct weapon_byteswithlength_s[1]){
					[0] =
						{
							.weapon_defs_count = 1,
							.weapon_defs =
								(struct weapon_defs_s[1]){
									[0] =
										{
											.id1 = 0,
											.id2 = 0,
											.weapon_group_id = 0,
											.flags1 = 0,
											.equip_ms = 0,
											.unequip_ms = 0,
											.from_passive_ms = 0,
											.to_passive_ms = 0,
											.xp_category = 0,
											.to_iron_sights_ms = 0,
											.from_iron_sights_ms = 0,
											.to_iron_sights_anim_ms = 0,
											.from_iron_sights_anim_ms = 0,
											.sprint_recovery_ms = 0,
											.next_use_delay_msec = 0,
											.turn_rate_modifier = 0,
											.movement_speed_modifier = 0,
											.propulsion_type = 0,
											.heat_bleed_off_rate = 0,
											.heat_capacity = 0,
											.overheat_penalty_ms = 0,
											.range_string_id = 0,
											.melee_detect_width = 0,
											.melee_detect_height = 0,

											.anim_set_name_length = 0,

											.vehicle_fp_camera_id = 0,
											.vehicle_tp_camera_id = 0,
											.overheat_effect_id = 0,
											.min_pitch = 0,
											.max_pitch = 0,
											.audio_game_object = 0,

											.ammo_slots_count = 1,
											.ammo_slots =
												(struct ammo_slots_s[1]){
													[0] =
														{
															.ammo_id = 0,
															.clip_size = 0,
															.capacity = 0,
															.start_empty = FALSE,
															.refill_ammo_per_tick = 0,
															.refill_ammo_delay_ms = 0,
															.clip_attachment_slot = 0,
															.clip_model_name_length = 0,
															.reload_weapon_bone_length = 0,
															.reload_character_bone_length = 0,
														},
												},

											.fire_groups_count = 1,
											.fire_groups =
												(struct fire_groups_s[1]){
													[0] =
														{
															.fire_group_id = 0,
														},
												},
										},
								},

							.fire_group_defs_count = 1,
							.fire_group_defs =
								(struct fire_group_defs_s[1]){
									[0] =
										{
											.id3 = 0,
											.id4 = 0,

											.fire_mode_list_count = 1,
											.fire_mode_list =
												(struct fire_mode_list_s[1]){
													[0] =
														{
															.fire_mode_1 = 0,
														},
												},

											.flags2 = 0,
											.chamber_duration_ms = 0,
											.image_set_override = 0,
											.transition_duration_ms = 0,
											.anim_actor_slot_override = 0,
											.deployable_id = 0,
											.spin_up_time_ms = 0,
											.spin_up_movement_modifier = 0,
											.spin_up_turn_rate_modifier = 0,
											.spool_up_time_ms = 0,
											.spool_up_initial_refire_ms = 0,
										},
								},

							.fire_mode_defs_count = 1,
							.fire_mode_defs =
								(struct fire_mode_defs_s[1]){
									[0] =
										{
											.id5 = 0,
											.id6 = 0,
											.flag1 = 0,
											.flag2 = 0,
											.flag3 = 0,
											.type = 0,
											.ammo_item_id = 0,
											.ammo_slot = 0,
											.burst_count = 0,
											.fire_duration_ms = 0,
											.fire_cooldown_duration_ms = 0,
											.refire_time_ms = 0,
											.auto_fire_time_ms = 0,
											.cook_time_ms = 0,
											.range = 0,
											.ammo_per_shot = 0,
											.reload_time_ms = 0,
											.reload_chamber_time_ms = 0,
											.reload_ammo_fill_time_ms = 0,
											.reload_loop_start_time_ms = 0,
											.reload_loop_end_time_ms = 0,
											.pellets_per_shot = 0,
											.pellet_spread = 0,
											.cof_recoil = 0,
											.cof_scalar = 0,
											.cof_scalar_moving = 0,
											.cof_override = 0,
											.recoil_angle_min = 0,
											.recoil_angle_max = 0,
											.recoil_horizontal_tolerance = 0,
											.recoil_horizontal_min = 0,
											.recoil_horizontal_max = 0,
											.recoil_magnitude_min = 0,
											.recoil_magnitude_max = 0,
											.recoil_recovery_delay_ms = 0,
											.recoil_recovery_rate = 0,
											.recoil_recovery_acceleration = 0,
											.recoil_shots_at_min_magnitude = 0,
											.recoil_max_total_magnitude = 0,
											.recoil_increase = 0,
											.recoil_increase_crouched = 0,
											.recoil_first_shot_modifier = 0,
											.recoil_horizontal_min_increase = 0,
											.recoil_horizontal_max_increase = 0,
											.fire_detect_range = 0,
											.effect_group = 0,
											.player_state_group_id = 0,
											.movement_modifier = 0,
											.turn_modifier = 0,
											.lock_on_icon_id = 0,
											.lock_on_angle = 0,
											.lock_on_radius = 0,
											.lock_on_range = 0,
											.lock_on_range_close = 0,
											.lock_on_range_far = 0,
											.lock_on_acquire_time_ms = 0,
											.lock_on_acquire_time_close_ms = 0,
											.lock_on_acquire_time_far_ms = 0,
											.lock_on_lose_time_ms = 0,
											.default_zoom = 0,
											.fp_offset_x = 0,
											.fp_offset_y = 0,
											.fp_offset_z = 0,
											.reticle_id = 0,
											.full_screen_effect = 0,
											.heat_per_shot = 0,
											.heat_threshold = 0,
											.heat_recovery_delay_ms = 0,
											.sway_amplitude_x = 0,
											.sway_amplitude_y = 0,
											.sway_period_x = 0,
											.sway_period_y = 0,
											.sway_initial_y_offset = 0,
											.arms_fov_scalar = 0,
											.anim_kick_magnitude = 0,
											.anim_recoil_magnitude = 0,
											.description_id = 0,
											.indirect_effect = 0,
											.bullet_arc_kick_angle = 0,
											.projectile_speed_override = 0,
											.inherit_from_id = 0,
											.inherit_from_charge_power = 0,
											.hud_image_id = 0,
											.target_requirement = 0,
											.fire_anim_duration_ms = 0,
											.sequential_fire_anim_start = 0,
											.sequential_fire_anim_count = 0,
											.cylof_recoil = 0,
											.cylof_scalar = 0,
											.cylof_scalar_moving = 0,
											.cylof_override = 0,
											.melee_composite_effect_id = 0,
											.melee_ability_id = 0,
											.sway_crouch_scalar = 0,
											.sway_prone_scalar = 0,
											.launch_pitch_additive_degrees = 0,
											.tp_force_camera_overrides = FALSE,
											.tp_camera_look_offset_x = 0,
											.tp_camera_look_offset_y = 0,
											.tp_camera_look_offset_z = 0,
											.tp_camera_position_offset_x = 0,
											.tp_camera_position_offset_y = 0,
											.tp_camera_position_offset_z = 0,
											.tp_camera_fov = 0,
											.fp_force_camera_overrides = FALSE,
											.tp_extra_lead_from_pitch_a = 0,
											.tp_extra_lead_from_pitch_b = 0,
											.tp_extra_lead_pitch_a = 0,
											.tp_extra_lead_pitch_b = 0,
											.tp_extra_height_from_pitch_a = 0,
											.tp_extra_height_from_pitch_b = 0,
											.tp_extra_height_pitch_a = 0,
											.tp_extra_height_pitch_b = 0,
											.fp_camera_fov = 0,
											.tp_cr_camera_look_offset_x = 0,
											.tp_cr_camera_look_offset_y = 0,
											.tp_cr_camera_look_offset_z = 0,
											.tp_cr_camera_position_offset_x = 0,
											.tp_cr_camera_position_offset_y = 0,
											.tp_cr_camera_position_offset_z = 0,
											.tp_pr_camera_look_offset_x = 0,
											.tp_pr_camera_look_offset_y = 0,
											.tp_pr_camera_look_offset_z = 0,
											.tp_pr_camera_position_offset_x = 0,
											.tp_pr_camera_position_offset_y = 0,
											.tp_pr_camera_position_offset_z = 0,
											.tp_cr_extra_lead_from_pitch_a = 0,
											.tp_cr_extra_lead_from_pitch_b = 0,
											.tp_cr_extra_lead_pitch_a = 0,
											.tp_cr_extra_lead_pitch_b = 0,
											.tp_cr_extra_height_from_pitch_a = 0,
											.tp_cr_extra_height_from_pitch_b = 0,
											.tp_cr_extra_height_pitch_a = 0,
											.tp_cr_extra_height_pitch_b = 0,
											.tp_pr_extra_lead_from_pitch_a = 0,
											.tp_pr_extra_lead_from_pitch_b = 0,
											.tp_pr_extra_lead_pitch_a = 0,
											.tp_pr_extra_lead_pitch_b = 0,
											.tp_pr_extra_height_from_pitch_a = 0,
											.tp_pr_extra_height_from_pitch_b = 0,
											.tp_pr_extra_height_pitch_a = 0,
											.tp_pr_extra_height_pitch_b = 0,
											.tp_camera_distance = 0,
											.tp_cr_camera_distance = 0,
											.tp_pr_camera_distance = 0,
											.tp_cr_camera_fov = 0,
											.tp_pr_camera_fov = 0,
											.fp_cr_camera_fov = 0,
											.fp_pr_camera_fov = 0,
											.force_fp_scope = FALSE, // note(doggo): might need to change this to TRUE later..
											.aim_assist_config = 0,
											.allow_depth_adjustment = FALSE,
											.tp_extra_draw_from_pitch_a = 0,
											.tp_extra_draw_from_pitch_b = 0,
											.tp_extra_draw_pitch_a = 0,
											.tp_extra_draw_pitch_b = 0,
											.tp_cr_extra_draw_from_pitch_a = 0,
											.tp_cr_extra_draw_from_pitch_b = 0,
											.tp_cr_extra_draw_pitch_a = 0,
											.tp_cr_extra_draw_pitch_b = 0,
											.tp_camera_pos_offset_y_mov = 0,
											.tp_camera_look_offset_y_mov = 0,
											.tp_cr_camera_pos_offset_y_mov = 0,
											.tp_cr_camera_look_offset_y_mov = 0,
											.tp_allow_move_heights = FALSE,
										},
								},

							.player_state_group_defs_count = 1,
							.player_state_group_defs =
								(struct player_state_group_defs_s[1]){
									[0] =
										{
											.id7 = 0,
											._id8 = 0,

											.player_state_properties_count = 1,
											.player_state_properties =
												(struct player_state_properties_s[1]){
													[0] =
														{
															.group_id = 0,
															.id9 = 0,
															.flags3 = 0,
															.min_cof = 0,
															.max_cof = 0,
															.cof_recovery_rate = 0,
															.cof_turn_penalty = 0,
															.shots_before_cof_penalty = 0,
															.cof_recovery_delay_threshold = 0,
															.cof_recovery_delay_ms = 0,
															.cof_grow_rate = 0,
															.min_cyl_of_fire = 0,
															.max_cyl_of_fire = 0,
															.cylof_recovery_rate = 0,
															.cylof_turn_penalty = 0,
															.shots_before_cylof_penalty = 0,
															.cylof_recovery_delay_threshold = 0,
															.cylof_recovery_delay_ms = 0,
															.cylof_grow_rate = 0,
														},
												},
										},
								},

							.fire_mode_projectile_mapping_data_count = 1,
							.fire_mode_projectile_mapping_data =
								(struct fire_mode_projectile_mapping_data_s[1]){
									[0] =
										{
											.id10 = 0,
											.id11 = 0,
											.index = 0,
											.projectile_definition_id = 0,
										},
								},

							.aim_assist_defs_count = 1,
							.aim_assist_defs =
								(struct aim_assist_defs_s[1]){
									[0] =
										{
											.id12 = 0,
											.cone_angle = 0,
											.cone_range = 0,
											.fall_off_cone_range = 0,
											.magnet_cone_angle = 0,
											.magnet_cone_range = 0,
											.target_override_delay = 0,
											.target_oos_delay = 0,
											.arrive_time = 0,
											.target_motion_update_time = 0,
											.weight = 0,
											.min_input_weight_delay_in = 0,
											.max_input_weight_delay_in = 0,
											.min_input_weight_delay_out = 0,
											.max_input_weight_delay_out = 0,
											.min_input_actor = 0,
											.max_input_actor = 0,
											.requirement_id = 0,
											.magnet_min_angle = 0,
											.magnet_dist_for_min_angle = 0,
											.magnet_max_angle = 0,
											.magnet_dist_for_max_angle = 0,
											.min_input_strafe_arrive_time = 0,
											.max_input_strafe_arrive_time = 0,
										},
								},
						},
				},
		};

	Zone_Packet_ClientGameSettings game_settings =
		{
			.interact_glow_and_dist = 16,
			.unk_bool = TRUE,
			.timescale = 1.0,
			.enable_weapons = 1,
			.unk_u32_2 = 1,
			.unk_float2 = 15.,
			.damage_multiplier = 11.,
		};

	Zone_Packet_SendSelfToClient send_self =
		{
			.payload_self =
				(struct payload_self_s[1]){
					[0] = {
						.guid = guid_get,
						.transient_id.value = transient_id_get,
						.actor_model_id = 9474,
						.head_actor_length = 26,
						.head_actor = "SurvivorFemale_Head_02.adr",
						.hair_model_length = 32,
						.hair_model = "SurvivorFemale_Hair_ShortBun.adr",
						.is_respawning = FALSE,
						.character_name_length = 5,
						.character_name = "doggo",
						.gender1 = 1 || 2,
						.creation_date = 0x133333333,
						.last_login_date = 0x133333333,

						.loadout_id = 3,
						.loadout_slots_array_count = 1,
						.loadout_slots_array =
							(struct loadout_slots_array_s[1]){
								[0] =
									{
										.hotbar_slot_id = 0,
										.loadout_id = 3,
										.slot_id = 0,
										.item_def_id4 = 1 || 0,
										.loadout_item_guid = 0x01 || 0,
										.unk_byte_17 = 255,
										.unk_dword_111 = 0,
									},
							},
						.current_slot_id = 7,

						.equipment_slots_count = 1,
						.equipment_slots =
							(struct equipment_slots_s[1]){
								[0] =
									{
										.unk_dword_7199 = 0,
										.unk_dword_890 = 0,
										.equipment_slot_id2 = 0,
										.equipment_slot_id3 = 0,
										.guid = 0x0,
										.decal_alias_length = 1,
										.decal_alias = "#"},
							},

						.character_resources_count = 1,
						.character_resources =
							(struct character_resources_s[1]){
								[0] =
									{
										.resource_type1 = 1,
										.resource_id = 1,
										.resource_type2 = 1,
										.value = 10000,
									},
							},

						.containers_count = 1,
						.containers =
							(struct containers_s[1]){
								[0] = {
									.loadout_slot_id = 0,
									.guid = item_guid_get,
									.def_id = 0,
									.associated_character_id = character_id_get,
									.slots = 14,
									.items2_count = 1,
									.items2 =
										(struct items2_s[1]){
											[0] =
												{
													.item_def_id5 = 0,
													.item_def_id6 = 0,
													.tint_id = 0,
													.guid = item_guid_get,
													.count = 14,
													.container_guid = item_guid_get,
													.container_def_id = 14,
													.container_slot_id = 0,
													.base_durability = 10,
													.current_durability = 10 ? 15 : 0,
													.max_durability_from_def = 10,
													.unk_bool_116 = TRUE,
													.owner_character_id = 0x0000000000000001,
													.unk_dword_9 = 1,
													// (doggo)put weapon data here from sendself later...
													.show_bulk = TRUE,
													.unk_dword_133 = 28,
												},
										},
								},
							},

						.is_admin = TRUE,
					},
				},
		};

	Zone_Packet_ContainerInitEquippedContainers init_equipped_containers =
		{
			.ignore_this = 0,
			.character_id = character_id_get,

			.container_list_count = 1,
			.container_list =
				(struct container_list_s[1]){
					[0] =
						{
							.loadout_slot_id = 0,

							.guid_1 = guid_get,
							.defs_id = 0,
							.associated_character_id = character_id_get,
							.slots = 0,

							.items_list_count = 1,
							.items_list =
								(struct items_list_s[1]){
									[0] =
										{
											.item_defs_id_1 = 0,

											.item_defs_id_2 = 0,
											.tint_id = 0,
											.guid_2 = guid_get,
											.count = 1,

											.unk_qword_1 = 0,
											.unk_dword_1 = 0,
											.unk_dword_2 = 0,

											.container_guid = 0,
											.contain_def_id = 0,
											.container_slot_id = 0,
											.base_durability = 0,
											.current_durability = 0,
											.max_durability_from_defs = 0,
											.unk_bool_1 = FALSE,
											.owner_character_id = character_id_get,
											.unk_dword_3 = 0,
										},
								},

							.show_bulk = TRUE,
							.max_bulk = 0,
							.unk_dword_4 = 0,
							.bulk_used = 0,
							.has_bulk_limit = TRUE,
						},
				},
		};

	Zone_Packet_UpdateWeatherData updt_weather_data =
		{
			.unknownDword1 = 1.0f,
			.fog_density = 0.0001733333f,
			.fog_floor = 10.0f,
			.fog_gradient = 0.0144f,
			.rain = 1.0f,
			.temp = 75.0f,
			.color_gradient = 0.0f,
			.unknown_dword8 = 0.05f,
			.unknown_dword9 = 0.0f,
			.unknown_dword10 = 0.05f,
			.unknown_dword11 = 0.15f,
			.unknown_dword12 = 0.0f,
			.sun_axis_x = 38.0f,
			.sun_axis_y = -15.0f,
			.unknown_dword15 = 0.0f,
			.disable_trees = -1.0f,
			.disable_trees1 = -0.05f,
			.disable_trees2 = -1.0f,
			.wind = 3.0f,
			.unknown_dword20 = 0.0f,
			.unknown_dword21 = 1.0f,

			.name_length = 16,
			.name = "sky_Z_clouds.dds",

			.unknown_dword22 = 0.3f,
			.unknown_dword23 = -0.002f,
			.unknown_dword24 = 0.0f,
			.unknown_dword25 = 1000.0f,
			.unknown_dword26 = 0.2f,
			.unknown_dword27 = 0.0f,
			.unknown_dword28 = 0.002f,
			.unknown_dword29 = 8000.0f,
			.ao_size = 0.0f,
			.ao_gamma = 0.25f,
			.ao_blackpoint = 7.0f,
			.unknown_dword33 = 0.5f,
		};

	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(10), Zone_Packet_Kind_InitializationParameters, &init_params);
	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(10), Zone_Packet_Kind_SendZoneDetails, &send_zone_details);
	//zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(40), Zone_Packet_Kind_CommandItemDefinitions, &item_defs);
	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(40), Zone_Packet_Kind_ReferenceDataWeaponDefinitions, &weapon_defs);
	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(10), Zone_Packet_Kind_ClientGameSettings, &game_settings);
	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(10), Zone_Packet_Kind_ContainerInitEquippedContainers, &init_equipped_containers);
	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(10), Zone_Packet_Kind_UpdateWeatherData, &updt_weather_data);
	zone_packet_send(app_state, session_state, &app_state->arena_per_tick, KB(500), Zone_Packet_Kind_SendSelfToClient, &send_self); // (doggo)if this packet was a person, I would beat the ever-living shit out of it!

	// zone_packet_raw_file_send(app_state, session_state, &app_state->arena_per_tick, KB(40), "C:\\Users\\epicg\\OneDrive\\Desktop\\send_self\\"
	// "5.bin");
}

internal void gateway_on_tunnel_data_from_client(App_State *app_state, Session_State *session_state, u8 *data, u32 data_length)
{
	zone_packet_handle(app_state, session_state, data, data_length);
}

internal INPUT_STREAM_CALLBACK_DATA(on_ping_input_stream_data)
{
	UNUSED(server);
	UNUSED(session);
	UNUSED(data);
	UNUSED(data_length);
	// ping_packet_handle(server, session, data, data_length);
}

internal INPUT_STREAM_CALLBACK_ACK(on_input_stream_ack)
{
	Session_State *session_state = session;
	session_state->ack_next = ack;
}

internal INPUT_STREAM_CALLBACK_DATA(on_input_stream_data)
{
	gateway_packet_handle(server, session, data, data_length);
}

internal OUTPUT_STREAM_CALLBACK_DATA(on_output_stream_data)
{
	App_State *app_state = server;
	Session_State *session_state = session;

	Data packet =
		{
			.sequence = (u16)sequence,
			.data = data,
			.data_length = data_length,
		};

	if (!is_fragment)
	{
		core_packet_send(app_state->socket, app_state->platform_api, session_state->address.ip, session_state->address.port, &session_state->connection_args, Core_Packet_Kind_Data, &packet);
	}
	else
	{
		core_packet_send(app_state->socket, app_state->platform_api, session_state->address.ip, session_state->address.port, &session_state->connection_args, Core_Packet_Kind_Data_Fragment, &packet);
	}
}

// TODO(rhett): STBSP_SPRINTF breaks if this is less than 4 bytes each? or sanitizer issue
char toggle_state_text[2][4] =
	{
		[FALSE] = "--",
		[TRUE] = "ON",
};

__declspec(dllexport) APP_TICK(server_tick)
{
	App_State *app_state = app_memory->app_state;
	if (!app_state)
	{
		app_state = app_memory->app_state = arena_bootstrap_push_struct(app_memory->backing_memory.data,
																		app_memory->backing_memory.size,
																		"Total",
																		App_State,
																		arena_total);
		app_state->platform_api = &app_memory->platform_api;
		app_state->tick_ms = &app_memory->tick_ms;
		app_state->work_ms = &app_memory->work_ms;
		app_state->tick_count = &app_memory->tick_count;
		app_state->key_states = &app_memory->key_states;

#if defined(TERMINAL_UI)
		Buffer screen =
			{
				.size = sizeof(char) * SCREEN_RESOLUTION,
				.data = arena_push_size(&app_state->arena_total,
										screen.size),
			};
		app_state->screen = app_memory->screen = screen;
		// core_memory_fill(app_state->screen.data, ' ', app_state->screen.size);
#endif // TERMINAL_UI

		Buffer per_tick_backing_memory =
			{
				.size = MB(10),
				.data = arena_push_size(&app_state->arena_total,
										per_tick_backing_memory.size),
			};

		app_state->stream_function_table = arena_push_struct(&app_state->arena_total, Stream_Function_Table);
		app_state->stream_function_table->game_input_ack = on_input_stream_ack;
		app_state->stream_function_table->game_input_data = on_input_stream_data;
		app_state->stream_function_table->game_output_data = on_output_stream_data;
		app_state->stream_function_table->ping_input_data = on_ping_input_stream_data;

		app_state->arena_per_tick =
			(Arena){
				.buffer = per_tick_backing_memory.data,
				.capacity = per_tick_backing_memory.size,
				.name = "Tick",
			};

		u8 rc4_key_encoded[] = "F70IaxuU8C/w7FPXY1ibXw==";
		app_state->rc4_key_decoded_length = util_base64_decode((u8 *)rc4_key_encoded,
															   sizeof(rc4_key_encoded) - 1,
															   app_state->rc4_key_decoded);

		app_state->connection_args.udp_length = MAX_PACKET_LENGTH;
		// TODO(rhett): encryption should probably be kept disabled initially and toggled on in higher layers
		// app_state->connection_args.use_encryption = FALSE;
		app_state->connection_args.should_dump_core = TRUE;
		app_state->connection_args.should_dump_login = TRUE;
		app_state->connection_args.should_dump_tunnel = TRUE;
		app_state->connection_args.should_dump_gateway = TRUE;
		app_state->connection_args.should_dump_zone = TRUE;

		app_state->sessions_capacity = MAX_SESSIONS_COUNT;
		app_state->socket = app_state->platform_api->socket_udp_create_and_bind(LOCAL_PORT);
		printf(MESSAGE_CONCAT_INFO("Game server socket bound to port " STRINGIFY(LOCAL_PORT) "\n\n"));
		app_state->platform_api->folder_create("packets");
	}

	if (should_reload)
	{
		printf(MESSAGE_CONCAT_INFO("Reloading function table...\n"));
		app_state->stream_function_table->game_input_ack = on_input_stream_ack;
		app_state->stream_function_table->game_input_data = on_input_stream_data;
		app_state->stream_function_table->game_output_data = on_output_stream_data;
		app_state->stream_function_table->ping_input_data = on_ping_input_stream_data;
	}

	// app_state->platform_api->buffer_write_to_file("STATE.dump",
	//(u8*)app_state,
	// sizeof(App_State));

	global_tick_count = *app_state->tick_count;

#if defined(TERMINAL_UI)
	core_memory_fill(app_state->screen.data, ' ', app_state->screen.size);
	stbsp_sprintf((char *)app_state->screen.data,
				  "Tick: %llu    %fms/w | %fms/f", *app_state->tick_count, *app_state->work_ms, *app_state->tick_ms);

	i32 pos = 0;
	for (i32 key = 0; key < 0xff; key++)
	{
		pos += stbsp_sprintf((char *)(app_state->screen.data + (SCREEN_WIDTH * 2) + pos),
							 "K%02x %s  ",
							 key,
							 toggle_state_text[(*app_state->key_states)[key]]);
	}

	// pos = SCREEN_WIDTH * 29;
	// pos += stbsp_sprintf((char*)(app_state->screen.data + pos),
	//"cool :)");
#endif // TERMINAL_UI

	u8 incoming_buffer[MAX_PACKET_LENGTH] = {0};
	u32 from_ip;
	u16 from_port;
	i32 receive_result = app_state->platform_api->receive_from(app_state->socket,
															   incoming_buffer,
															   MAX_PACKET_LENGTH,
															   &from_ip,
															   &from_port);

	if (receive_result)
	{
		printf("\n\nPacket Tick Begin ============================================================\\\\\n");

		Session_Address incoming_session_address =
			{
				.ip = from_ip,
				.port = from_port,
			};

		// TODO(rhett): Before we tackle the problem of multiple clients, let's reliably handle one first
		// TODO(rhett): will need cleaned up
		i32 first_free_session = -1;
		i32 known_session = -1;
		for (i32 i = 0; i < app_state->sessions_capacity; i++)
		{
			if (first_free_session == -1 && !app_state->sessions[i].address.full)
			{
				first_free_session = i;
			}

			if (known_session == -1 && incoming_session_address.full == app_state->sessions[i].address.full)
			{
				known_session = i;
			}

			if (first_free_session != -1 && known_session != -1)
			{
				break;
			}
		}

		if (core_packet_get_kind(incoming_buffer, receive_result) == Core_Packet_Kind_Session_Request)
		{
			if (known_session != -1)
			{
				printf(MESSAGE_CONCAT_INFO("Known client %u.%u.%u.%u:%u re-sent SessionRequest\n"),
					   (from_ip & 0xff000000) >> 24,
					   (from_ip & 0x00ff0000) >> 16,
					   (from_ip & 0x0000ff00) >> 8,
					   (from_ip & 0x000000ff),
					   from_port);
			}
			else
			{
				printf(MESSAGE_CONCAT_INFO("Unknown client %u.%u.%u.%u:%u sent SessionRequest. Beginning session\n"),
					   (from_ip & 0xff000000) >> 24,
					   (from_ip & 0x00ff0000) >> 16,
					   (from_ip & 0x0000ff00) >> 8,
					   (from_ip & 0x000000ff),
					   from_port);

				if (first_free_session == -1)
				{
					printf(MESSAGE_CONCAT_WARN("No free sessions avaliable\n"));
				}
				else
				{
					known_session = first_free_session;
					app_state->sessions[first_free_session].address.full = incoming_session_address.full;
					app_state->sessions[first_free_session].ack_next = -1;
					app_state->sessions[first_free_session].ack_previous = -1;

					base_memory_copy((void *)&app_state->sessions[first_free_session].connection_args,
									 (void *)&app_state->connection_args,
									 sizeof(app_state->connection_args));

					// TODO(rhett): These fragment pools will leak memory. this whole system needs refactored
					app_state->sessions[first_free_session].input_fragment_pool = fragment_pool_create(MAX_FRAGMENTS,
																									   MAX_PACKET_LENGTH,
																									   &app_state->arena_total);
					app_state->sessions[first_free_session].output_fragment_pool = fragment_pool_create(MAX_FRAGMENTS,
																										MAX_PACKET_LENGTH - DATA_HEADER_LENGTH,
																										&app_state->arena_total);
					app_state->sessions[first_free_session].input_stream = input_stream_init(&app_state->sessions[first_free_session].input_fragment_pool,
																							 app_state->rc4_key_decoded,
																							 app_state->rc4_key_decoded_length,
																							 FALSE);
					app_state->sessions[first_free_session].output_stream = output_stream_init(&app_state->sessions[first_free_session].output_fragment_pool,
																							   app_state->rc4_key_decoded,
																							   app_state->rc4_key_decoded_length,
																							   FALSE);

					app_state->sessions[first_free_session].input_stream.ack_callback_ptr = &app_state->stream_function_table->game_input_ack;
					app_state->sessions[first_free_session].input_stream.data_callback_ptr = &app_state->stream_function_table->game_input_data;
					app_state->sessions[first_free_session].output_stream.data_callback_ptr = &app_state->stream_function_table->game_output_data;
				}
			}
		}

		if (known_session != -1)
		{
			if (app_state->sessions[known_session].kind == Session_Kind_Ping_Responder)
			{
				// ping_packet_handle(app_state,
				//&app_state->sessions[known_session],
				// incoming_buffer,
				// receive_result);
			}
			else
			{
				core_packet_handle(app_state, app_state->platform_api, &app_state->sessions[known_session], incoming_buffer, receive_result, FALSE);
			}

			// Zone_Packet_ClientUpdatePacketModifyMovementSpeed speed = { .speed = 4.0f, .unk_bool = 1};

			// local_persist b32 do_once_7 = 0;
			// if (!do_once_7++)
			//{
			// DO_ONCE(printf("AAAAAAAAAAAAAAAAA\n\n"); zone_packet_send(app_state, &app_state->sessions[known_session], &app_state->arena_per_tick, KB(1), Zone_Packet_Kind_ClientUpdatePacketModifyMovementSpeed, &speed););
			// }

			if (app_state->sessions[known_session].ack_previous != app_state->sessions[known_session].ack_next)
			{
				printf(MESSAGE_CONCAT_INFO("Syncing ack...\n"));
				app_state->sessions[known_session].ack_previous = app_state->sessions[known_session].ack_next;

				Ack ack =
					{
						.sequence = (u16)app_state->sessions[known_session].ack_next};

				core_packet_send(app_state->socket,
								 app_state->platform_api,
								 app_state->sessions[known_session].address.ip,
								 app_state->sessions[known_session].address.port,
								 &app_state->sessions[known_session].connection_args,
								 Core_Packet_Kind_Ack,
								 &ack);
			}
		}

		////// TODO(rhett): SHELVING THIS FOR NOW
		// TODO(rhett): think about when we should process our packet queue
		// TODO(rhett): we also should have a way to time this,
		//              probably through the platform api
		// TODO(rhett): processing one normal packet per tick for now
		// if (app_state->packet_queue.entries_tail > 0)
		//{
		// packet_queue_pop_and_send(&app_state->packet_queue,
		//                     app_state);
		//                     //&app_state->arena_per_frame,
		//                     //app_state->server_packet_send);
		//
		////Packet_Queue_Entry normal_packet_entry = packet_queue_pop(&app_state->packet_queue);
		////zone_packet_send(app_state,
		//			//normal_packet_entry.session_state,
		//			//&app_state->arena_per_frame,
		//			//normal_packet_entry.max_packed_length,
		//			//normal_packet_entry.packet_kind,
		//			//app_state->packet_queue.buffer + normal_packet_entry.buffer_offset);
		//}

		printf("Packet Tick End ==============================================================//\n");
	}

	arena_reset(&app_state->arena_per_tick);
}
