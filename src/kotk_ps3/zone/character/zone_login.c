void onZoneLogin(App_State *app, Session_State *session)
{
	Zone_Packet_InitializationParameters init_params =
		{
			.environment_length = 9,
			.environment = "LIVE_KOTK",
		};
	zone_packet_send(app, session, &app->arena_per_tick, KB(10), Zone_Packet_Kind_InitializationParameters, &init_params);

	Zone_Packet_SendZoneDetails send_zone_details =
		{
			.zone_name_length = 9,
			.zone_name = "LoginZone",
			.zone_type = 4,
			.unk_bool = false,

			.overcast = 0,
			.fogDensity = 0,
			.fogFloor = 14.8f,
			.fogGradient = 15.25f,
			.globalPrecipitation = 0,
			.temperature = 75,
			.skyClarity = 0,
			.cloudWeight0 = 0.16f,
			.cloudWeight1 = 0.16f,
			.cloudWeight2 = 0.13f,
			.cloudWeight3 = 0.13f,
			.transitionTime = 0,
			.sunAxisX = 40,
			.sunAxisY = 0,
			.sunAxisZ = 0,
			.windDirX = -1.0f,
			.windDirY = -0.5f,
			.windDirZ = 1.0f,
			.wind = 3,
			.rainMinStrength = 0,
			.rainRampUpTimeSeconds = 1,
			.cloudFile_length = 16,
			.cloudFile = "sky_Z_clouds.dds",
			.stratusCloudTiling = 0.2f,
			.stratusCloudScrollU = -0.002f,
			.stratusCloudScrollV = 0,
			.stratusCloudHeight = 1000,
			.cumulusCloudTiling = 0.2f,
			.cumulusCloudScrollU = 0,
			.cumulusCloudScrollV = 0.002f,
			.cumulusCloudHeight = 8000,
			.cloudAnimationSpeed = 0,
			.cloudSilverLiningThickness = 0.39f,
			.cloudSilverLiningBrightness = 0.5f,
			.cloudShadows = 0.2f,

			.zone_id = 5,
			.zone_id_2 = 5,
			.name_id = 7699,
			.unk_bool2 = true,
			.lighting_length = 15,
			.lighting = "Lighting_Z2.txt",
			.unk_bool3 = false,
			.unk_bool4 = false,
		};
	zone_packet_send(app, session, &app->arena_per_tick, KB(10), Zone_Packet_Kind_SendZoneDetails, &send_zone_details);

	Zone_Packet_ClientGameSettings game_settings =
		{
			.interact_glow_and_dist = 16,
			.unk_bool = true,
			.timescale = 1.0,
			.enable_weapons = 1,
			.unk_u32_2 = 1,
			.unk_float2 = 15.,
			.damage_multiplier = 11.,
		};
	zone_packet_send(app, session, &app->arena_per_tick, KB(10), Zone_Packet_Kind_ClientGameSettings, &game_settings);

	Zone_Packet_UpdateWeatherData updt_weather_data =
		{
			.overcast = 0,
			.fogDensity = 0,
			.fogFloor = 14.8f,
			.fogGradient = 0.0144f,
			.globalPrecipitation = 0,
			.temperature = 75,
			.skyClarity = 0,
			.cloudWeight0 = 0.16f,
			.cloudWeight1 = 0.16f,
			.cloudWeight2 = 0.13f,
			.cloudWeight3 = 0.13f,
			.transitionTime = 0,
			.sunAxisX = 40,
			.sunAxisY = 0,
			.sunAxisZ = 0,
			.windDirX = -1.0f,
			.windDirY = -0.5f,
			.windDirZ = 1.0f,
			.wind = 3,
			.rainMinStrength = 0,
			.rainRampUpTimeSeconds = 0,
			.cloudFile_length = 16,
			.cloudFile = "sky_Z_clouds.dds",
			.stratusCloudTiling = 0.2f,
			.stratusCloudScrollU = -0.002f,
			.stratusCloudScrollV = 0,
			.stratusCloudHeight = 1000,
			.cumulusCloudTiling = 0.2f,
			.cumulusCloudScrollU = 0,
			.cumulusCloudScrollV = 0.002f,
			.cumulusCloudHeight = 8000,
			.cloudAnimationSpeed = 0,
			.cloudSilverLiningThickness = 0.39f,
			.cloudSilverLiningBrightness = 0.5f,
			.cloudShadows = 0.2f,
		};
	zone_packet_send(app, session, &app->arena_per_tick, KB(10), Zone_Packet_Kind_UpdateWeatherData, &updt_weather_data);

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
															.start_empty = false,
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
											.tp_force_camera_overrides = false,
											.tp_camera_look_offset_x = 0,
											.tp_camera_look_offset_y = 0,
											.tp_camera_look_offset_z = 0,
											.tp_camera_position_offset_x = 0,
											.tp_camera_position_offset_y = 0,
											.tp_camera_position_offset_z = 0,
											.tp_camera_fov = 0,
											.fp_force_camera_overrides = false,
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
											.force_fp_scope = false, // note(doggo): might need to change this to true later..
											.aim_assist_config = 0,
											.allow_depth_adjustment = false,
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
											.tp_allow_move_heights = false,
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
	zone_packet_send(app, session, &app->arena_per_tick, KB(40), Zone_Packet_Kind_ReferenceDataWeaponDefinitions, &weapon_defs);

	loadCharacterData(app, session);
	// pGetLightWeight(app, session);
}