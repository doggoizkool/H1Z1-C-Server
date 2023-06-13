// TODO(rhett): is ID always a byte?
#define GATEWAY_PACKET_ID_SIZE        1
#define GATEWAY_PACKET_RESERVED_SIZE  (CORE_DATA_FRAGMENT_EXTRA_SIZE)

#define GATEWAY_PACKET_UNPACK_ID(opcode)              (opcode & 0x1F)
#define GATEWAY_PACKET_UNPACK_CHANNEL(opcode)         (opcode >> 5)
#define GATEWAY_PACKET_PACK_ID(opcode, id)            (opcode | (id & 0x1F))
#define GATEWAY_PACKET_PACK_CHANNEL(opcode, channel)  (opcode | (channel << 5))

// NOTE(rhett): INVALID or UNHANDLED?
#define GATEWAY_PACKET_KINDS \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_Invalid,                             0xff,  "INVALID"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_LoginRequest,                        0x01,  "LoginRequest"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_LoginReply,                          0x02,  "LoginReply"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_Logout,                              0x03,  "Logout"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_ForceDisconnect,                     0x04,  "ForceDisconnect"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_TunnelPacketToExternalConnection,    0x05,  "TunnelPacketToExternalConnection"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_TunnelPacketFromExternalConnection,  0x06,  "TunnelPacketFromExternalConnection"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_ChannelIsRoutable,                   0x07,  "ChannelIsRoutable"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_ConnectionIsNotRoutable,             0x08,  "ConnectionIsNotRoutable"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind__End,                                0xff,  "")

		typedef enum Gateway_Packet_Kind
	{
#define GATEWAY_PACKET_KIND(kind, id, str) kind
		GATEWAY_PACKET_KINDS
#undef GATEWAY_PACKET_KIND
	} Gateway_Packet_Kind;

	// NOTE(rhett): Invalid must be falsey
	STATIC_ASSERT(!Gateway_Packet_Kind_Invalid);

	// NOTE(rhett): for remedybg stuff
	global Gateway_Packet_Kind global_gateway_packet_kinds[Gateway_Packet_Kind__End + 1] =
	{
#define GATEWAY_PACKET_KIND(kind, id, str) [kind] = kind
		GATEWAY_PACKET_KINDS
#undef GATEWAY_PACKET_KIND
	};

	// TODO(rhett): + 1 for here and not when used in iter? to skip __End?
	global u8 global_gateway_packet_ids[Gateway_Packet_Kind__End + 1] =
	{
#define GATEWAY_PACKET_KIND(kind, id, str) [kind] = id
		GATEWAY_PACKET_KINDS
#undef GATEWAY_PACKET_KIND
	};

	global char* global_gateway_packet_names[Gateway_Packet_Kind__End + 1] =
	{
#define GATEWAY_PACKET_KIND(kind, id, str) [kind] = str
		GATEWAY_PACKET_KINDS
#undef GATEWAY_PACKET_KIND
	};

	// TODO(rhett): Do I like this?
	global i32 global_gateway_packet_begin = Gateway_Packet_Kind_Invalid + 1;
	global i32 global_gateway_packet_count = Gateway_Packet_Kind__End;


	typedef struct Gateway_Packet_LoginRequest Gateway_Packet_LoginRequest;
	struct Gateway_Packet_LoginRequest
	{
		u64 character_id;
		u32 server_ticket_length;
		char* server_ticket;
		u32 client_protocol_length;
		char* client_protocol;
		u32 client_build_length;
		char* client_build;
	};

	typedef struct Gateway_Packet_LoginReply Gateway_Packet_LoginReply;
	struct Gateway_Packet_LoginReply
	{
		b8 is_logged_in;
	};

	typedef struct Gateway_Packet_TunnelPacket Gateway_Packet_TunnelPacket;
	struct Gateway_Packet_TunnelPacket
	{
		u8 channel;

		u32 data_size;
		u8* data;
	};

	typedef struct Gateway_Packet_ChannelIsRoutable Gateway_Packet_ChannelIsRoutable;
	struct Gateway_Packet_ChannelIsRoutable
	{
		u8 channel;

		b8 is_routable;
		b8 unk_bool;
	};


	void protocol_gateway_packet_send(void* packet_ptr,
	                                  Gateway_Packet_Kind packet_kind,
	                                  Session_Handle session_handle,
	                                  App_State* app_state)
	{
		// TODO(rhett): will a gateway packet ever excede max fragment size?
		u8 buffer[MAX_PACKET_SIZE] = { 0 };
		Stream output_stream =
		{
			.size = SIZE_OF(buffer),
			.data = buffer,
			.cursor = GATEWAY_PACKET_RESERVED_SIZE,
		};

		// NOTE(rhett): packing
		u8 packet_channel = 0;
		switch (packet_kind)
		{
			case Gateway_Packet_Kind_LoginReply:
			{
				Gateway_Packet_LoginReply* login_reply = packet_ptr;

				stream_write_u8_little(output_stream, global_gateway_packet_ids[packet_kind]);
				stream_write_u8_little(output_stream, login_reply->is_logged_in);
			} break;

			case Gateway_Packet_Kind_TunnelPacketToExternalConnection: fallthrough;
			case Gateway_Packet_Kind_TunnelPacketFromExternalConnection:
			{
				NOT_IMPLEMENTED_MSG("not packing + sending from here anymore");
			} break;

			case Gateway_Packet_Kind_ChannelIsRoutable:
			{
				Gateway_Packet_ChannelIsRoutable* channel_is_routable = packet_ptr;
				packet_channel = channel_is_routable->channel;

				stream_write_u8_little(output_stream, GATEWAY_PACKET_PACK_CHANNEL(global_gateway_packet_ids[packet_kind], channel_is_routable->channel));
				stream_write_u8_little(output_stream, channel_is_routable->is_routable);
				stream_write_u8_little(output_stream, channel_is_routable->unk_bool);
			} break;

			default:
			{
				NOT_IMPLEMENTED_MSG("sending not handled");
			}
		}

		printf(MESSAGE_CONCAT_INFO("Sending %s on channel %u...\n"),
		       global_gateway_packet_names[packet_kind],
		       packet_channel);

		if (global_should_dump_core)
		{
			app_state->platform_api->folder_create(PACKET_FOLDER);
			char dump_path[64] = { 0 };
			stbsp_snprintf(dump_path,
			               SIZE_OF(dump_path),
			               PACKET_FOLDER "\\%llu_%llu_S_gateway_%u_%s.bin",
			               global_tick_count,
			               global_dump_count++,
			               packet_channel,
			               global_gateway_packet_names[packet_kind]);
			app_state->platform_api->buffer_write_to_file(dump_path,
			                                              (u8*)output_stream.data + GATEWAY_PACKET_RESERVED_SIZE,
			                                              cast(u32)output_stream.cursor - GATEWAY_PACKET_RESERVED_SIZE);
		}

		b32 ignore_encryption = FALSE;
		if (packet_kind == Gateway_Packet_Kind_LoginReply)
		{
			//ignore_encryption = TRUE;
		}
		protocol_core_data_send((Buffer){.size = output_stream.cursor, .data = output_stream.data}, ignore_encryption, session_handle, app_state);
	}

	// TODO(rhett): even use i/usize? just typedef myself in case of 32-bit? support 32-bit?
	// TODO(rhett): I don't like cast from isize to u32 and back so much
	// IMPORTANT(rhett): Assumes buffer reserves space lower layers
	void protocol_gateway_tunnel_data_send(Buffer tunnel_data_buffer,
	                                       Session_Handle session_handle,
	                                       App_State* app_state)
	{
		// TODO(rhett): do we ever send on other channels?
		endian_write_u8_little(GATEWAY_PACKET_RESERVED_SIZE + tunnel_data_buffer.data, global_gateway_packet_ids[Gateway_Packet_Kind_TunnelPacketToExternalConnection]);
		if (global_should_dump_core)
		{
			app_state->platform_api->folder_create(PACKET_FOLDER);
			char dump_path[96] = { 0 };
			stbsp_snprintf(dump_path,
			               SIZE_OF(dump_path),
			               // TODO(rhett): hardcoded channel 0
			               PACKET_FOLDER "\\%llu_%llu_S_tunnel_0_data.bin",
			               global_tick_count,
			               global_dump_count++);
			app_state->platform_api->buffer_write_to_file(dump_path,
			                                              GATEWAY_PACKET_RESERVED_SIZE + tunnel_data_buffer.data,
			                                              cast(u32)tunnel_data_buffer.size - GATEWAY_PACKET_RESERVED_SIZE);
		}
		protocol_core_data_send(tunnel_data_buffer, FALSE, session_handle, app_state);
	}

	// NOTE(rhett): unpacking with temp arena. copy anything important
	Stream protocol_gateway_packet_unpack(Stream packet_stream, void* result_ptr, Gateway_Packet_Kind packet_kind, Protocol_Options protocol_options, Arena* arena)
	{
		ASSERT(packet_stream.data);
		ASSERT(packet_stream.size);
		ASSERT(packet_stream.cursor == GATEWAY_PACKET_ID_SIZE);
		ASSERT(result_ptr);
		// NOTE(rhett): valid oughta be truthy
		ASSERT(packet_kind);

		switch (packet_kind)
		{
			case Gateway_Packet_Kind_LoginRequest:
			{
				Gateway_Packet_LoginRequest* packet = result_ptr;

				packet->character_id = stream_read_u64_little(packet_stream);
				packet->server_ticket_length = stream_read_u32_little(packet_stream);
				packet->server_ticket = arena_push_size(arena, packet->server_ticket_length);
				for (u32 server_ticket_iter = 0; server_ticket_iter < packet->server_ticket_length; server_ticket_iter++)
				{
					packet->server_ticket[server_ticket_iter] = stream_read_u8_little(packet_stream);
				}

				packet->client_protocol_length = stream_read_u32_little(packet_stream);
				packet->client_protocol = arena_push_size(arena, packet->client_protocol_length);
				for (u32 client_protocol_iter = 0; client_protocol_iter < packet->client_protocol_length; client_protocol_iter++)
				{
					packet->client_protocol[client_protocol_iter] = stream_read_u8_little(packet_stream);
				}

				packet->client_build_length = stream_read_u32_little(packet_stream);
				packet->client_build = arena_push_size(arena, packet->client_build_length);
				for (u32 client_build_iter = 0; client_build_iter < packet->client_build_length; client_build_iter++)
				{
					packet->client_build[client_build_iter] = stream_read_u8_little(packet_stream);
				}
			} break;

			default:
			{
				NOT_IMPLEMENTED_MSG("unpacking not handled");
			}
		}

		return packet_stream;
	}

#define GATEWAY_LOGIN_CALLBACK(name)  void name(Session_Handle session_handle, App_State* app_state)
	GATEWAY_LOGIN_CALLBACK(on_gateway_login);

#define GATEWAY_TUNNEL_DATA_CALLBACK(name)  void name(Gateway_Packet_TunnelPacket tunnel_data, Session_Handle session_handle, App_State* app_state)
	GATEWAY_TUNNEL_DATA_CALLBACK(on_gateway_tunnel_data);

	void protocol_gateway_packet_route(Buffer packet_buffer, Session_Handle session_handle, App_State* app_state)
	{
		ASSERT(app_state);
		ASSERT(session_handle.id);
		ASSERT(packet_buffer.data);

		Stream packet_stream =
		{
			.size = packet_buffer.size,
			.data = packet_buffer.data,
		};

		u8 gateway_header = stream_read_u8_big(packet_stream);
		u8 packet_id = GATEWAY_PACKET_UNPACK_ID(gateway_header);
		u8 packet_channel = GATEWAY_PACKET_UNPACK_CHANNEL(gateway_header);

		// TEMP(rhett): 
		if (packet_channel > 1 || packet_id > 0x8)
		{
			printf(MESSAGE_CONCAT_WARN("skipping gateway packet\n"));
			return;
		}

		Gateway_Packet_Kind packet_kind = Gateway_Packet_Kind_Invalid;
		// TODO(rhett): Skip Invalid and __End?
		for (i32 kind_iter = global_gateway_packet_begin; kind_iter < global_gateway_packet_count; kind_iter++)
		{
			if (packet_id == global_gateway_packet_ids[kind_iter])
			{
				packet_kind = kind_iter;
				break;
			}
		}
		// NOTE(rhett): invalid should be falsey
		//if (!packet_kind)
		//{
		//NOT_IMPLEMENTED_MSG("Invalid gateway packet");
		//}

		// TODO(rhett): just dumping everything right now
		if (global_should_dump_core)
		{
			app_state->platform_api->folder_create(PACKET_FOLDER);
			char dump_path[96] = { 0 };
			stbsp_snprintf(dump_path,
			               SIZE_OF(dump_path),
			               PACKET_FOLDER "\\%llu_%llu_C_gateway_%u_%s.bin",
			               global_tick_count,
			               global_dump_count++,
			               packet_channel,
			               global_gateway_packet_names[packet_kind]);
			app_state->platform_api->buffer_write_to_file(dump_path,
			                                              packet_buffer.data,
			                                              cast(u32)packet_buffer.size);
		}

		if (ignore_packets) return; // TEMP(rhett):

		printf(MESSAGE_CONCAT_INFO("Routing %s on channel %u...\n"), global_gateway_packet_names[packet_kind], packet_channel);
		Session_State* session = session_get_pointer_from_handle(&app_state->session_pool, session_handle);
		switch (packet_kind)
		{
			case Gateway_Packet_Kind_LoginRequest:
			{
				Gateway_Packet_LoginRequest packet = { 0 };
				packet_stream = protocol_gateway_packet_unpack(packet_stream, &packet, packet_kind, session->protocol_options, &app_state->arena_per_tick);

				printf(MESSAGE_CONCAT_INFO("Toggling encryption\n"));
				session->protocol_options.use_encryption = TRUE;

				Gateway_Packet_LoginReply reply =
				{
					.is_logged_in = TRUE,
				};
				protocol_gateway_packet_send(&reply,
				                             Gateway_Packet_Kind_LoginReply,
				                             session_handle,
				                             app_state);

				Gateway_Packet_ChannelIsRoutable channel_is_routable_0 =
				{
					.channel = 0,
					.is_routable = TRUE,
				};
				protocol_gateway_packet_send(&channel_is_routable_0,
				                             Gateway_Packet_Kind_ChannelIsRoutable,
				                             session_handle,
				                             app_state);

				Gateway_Packet_ChannelIsRoutable channel_is_routable_1 =
				{
					.channel = 1,
					.is_routable = TRUE,
				};
				protocol_gateway_packet_send(&channel_is_routable_1,
				                             Gateway_Packet_Kind_ChannelIsRoutable,
				                             session_handle,
				                             app_state);

				on_gateway_login(session_handle, app_state);

			} break;

			case Gateway_Packet_Kind_TunnelPacketFromExternalConnection:
			{
				Gateway_Packet_TunnelPacket tunnel_packet =
				{
					.channel = packet_channel,
					.data = (u8*)packet_stream.data + packet_stream.cursor,
					.data_size = cast(u32)packet_stream.size - cast(u32)packet_stream.cursor,
				};
				packet_stream.cursor += cast(u32)packet_stream.size - cast(u32)packet_stream.cursor;

				on_gateway_tunnel_data(tunnel_packet, session_handle, app_state);
			} break;

			default:
			{
				//NOT_IMPLEMENTED_MSG("routing not handled");
				return;
			}
		}

		ASSERT_MSG(packet_stream.cursor == packet_stream.size, "There is more data left?");
	}

#if 0

#define TUNNEL_DATA_HEADER_LENGTH 1

#define GATEWAY_LOGINREQUEST_ID 0x1
#define GATEWAY_LOGINREPLY_ID 0x2
#define GATEWAY_TUNNELPACKETTOEXTERNALCONNECTION_ID 0x5
#define GATEWAY_TUNNELPACKETFROMEXTERNALCONNECTION_ID 0x6
#define GATEWAY_CHANNELISROUTABLE_ID 0x7

#define GATEWAY_PACKET_KINDS \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_Unhandled, "Unhandled"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_LoginRequest, "LoginRequest"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_LoginReply, "LoginReply"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_TunnelPacketToExternalConnection, "TunnelPacketToExternalConnection"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_TunnelPacketFromExternalConnection, "TunnelPacketFromExternalConnection"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind_ChannelIsRoutable, "ChannelIsRoutable"), \
	GATEWAY_PACKET_KIND(Gateway_Packet_Kind__End, "")

typedef enum Gateway_Packet_Kind
{
#define GATEWAY_PACKET_KIND(e, s) e
	GATEWAY_PACKET_KINDS
#undef GATEWAY_PACKET_KIND
} Gateway_Packet_Kind;

char* gateway_packet_names[Gateway_Packet_Kind__End + 1] =
{
#define GATEWAY_PACKET_KIND(e, s) s
	GATEWAY_PACKET_KINDS
#undef GATEWAY_PACKET_KIND
};


typedef struct Gateway_Packet_LoginRequest Gateway_Packet_LoginRequest;
struct Gateway_Packet_LoginRequest
{
	u64 character_id;
	u32 server_ticket_length;
	char* server_ticket;
	u32 client_protocol_length;
	char* client_protocol;
	u32 client_build_length;
	char* client_build;
};

typedef struct Gateway_Packet_LoginReply Gateway_Packet_LoginReply;
struct Gateway_Packet_LoginReply
{
	b8 is_logged_in;
};

typedef struct Gateway_Packet_TunnelPacket Gateway_Packet_TunnelPacket;
struct Gateway_Packet_TunnelPacket
{
	u8 channel;

	u32 data_length;
	u8* data;
};

typedef struct Gateway_Packet_ChannelIsRoutable Gateway_Packet_ChannelIsRoutable;
struct Gateway_Packet_ChannelIsRoutable
{
	u8 channel;

	b8 is_routable;
	b8 unk_bool;
};


internal u32 gateway_packet_pack(Gateway_Packet_Kind packet_kind,
                                 void* packet_ptr,
                                 u8* buffer)
{
	u32 offset = 0;

	printf("\n");
	switch (packet_kind)
	{
		case Gateway_Packet_Kind_LoginRequest:
		{
			printf("[*] Packing LoginRequest...\n");
			Gateway_Packet_LoginRequest* packet = packet_ptr;

			endian_write_u8_little(buffer + offset, GATEWAY_LOGINREQUEST_ID);
			offset++;

			// u64 character_id
			endian_write_u64_little(buffer + offset, packet->character_id);
			offset += sizeof(u64);
			printf("-- character_id            \t%lld\t%llxh\t%f\n", (i64)packet->character_id, (u64)packet->character_id, (f64)packet->character_id);

			// string server_ticket
			endian_write_u32_little(buffer + offset, packet->server_ticket_length);
			offset += sizeof(u32);
			printf("-- STRING_LENGTH           \t%lld\t%llxh\t%f\n", (i64)packet->server_ticket_length, (u64)packet->server_ticket_length, (f64)packet->server_ticket_length);
			for (u32 server_ticket_iter = 0; server_ticket_iter < packet->server_ticket_length; server_ticket_iter++)
			{
				endian_write_i8_little(buffer + offset, packet->server_ticket[server_ticket_iter]);
				offset++;
			}

			// string client_protocol
			endian_write_u32_little(buffer + offset, packet->client_protocol_length);
			offset += sizeof(u32);
			printf("-- STRING_LENGTH           \t%lld\t%llxh\t%f\n", (i64)packet->client_protocol_length, (u64)packet->client_protocol_length, (f64)packet->client_protocol_length);
			for (u32 client_protocol_iter = 0; client_protocol_iter < packet->client_protocol_length; client_protocol_iter++)
			{
				endian_write_i8_little(buffer + offset, packet->client_protocol[client_protocol_iter]);
				offset++;
			}

			// string client_build
			endian_write_u32_little(buffer + offset, packet->client_build_length);
			offset += sizeof(u32);
			printf("-- STRING_LENGTH           \t%lld\t%llxh\t%f\n", (i64)packet->client_build_length, (u64)packet->client_build_length, (f64)packet->client_build_length);
			for (u32 client_build_iter = 0; client_build_iter < packet->client_build_length; client_build_iter++)
			{
				endian_write_i8_little(buffer + offset, packet->client_build[client_build_iter]);
				offset++;
			}

		} break;

		case Gateway_Packet_Kind_LoginReply:
		{
			printf("[*] Packing LoginReply...\n");
			Gateway_Packet_LoginReply* packet = packet_ptr;

			endian_write_u8_little(buffer + offset, GATEWAY_LOGINREPLY_ID);
			offset++;

			// b8 is_logged_in
			endian_write_b8_little(buffer + offset, packet->is_logged_in);
			offset += sizeof(b8);
			printf("-- is_logged_in            \t%lld\t%llxh\t%f\n", (i64)packet->is_logged_in, (u64)packet->is_logged_in, (f64)packet->is_logged_in);

		} break;

		case Gateway_Packet_Kind_TunnelPacketToExternalConnection:
		case Gateway_Packet_Kind_TunnelPacketFromExternalConnection:
		{
			// NOTE(rhett): To avoid allocating a buffer for a full packet twice,
			//              assume the passed buffer already includes the tunnel data
			printf("[*] Packing %s...\n", gateway_packet_names[packet_kind]);
			Gateway_Packet_TunnelPacket* packet = packet_ptr;

			u8 opcode;
			if (packet_kind == Gateway_Packet_Kind_TunnelPacketToExternalConnection)
			{
				opcode = GATEWAY_TUNNELPACKETTOEXTERNALCONNECTION_ID;
			}
			else if (packet_kind == Gateway_Packet_Kind_TunnelPacketFromExternalConnection)
			{
				opcode = GATEWAY_TUNNELPACKETFROMEXTERNALCONNECTION_ID;
			}
			else
			{
				printf("[X] Packing unknown TunnelPacket\n");
				abort();
			}

			endian_write_u8_little(buffer + offset, opcode | (packet->channel << 5));
			offset++;
			printf("-- channel                 \t%lld\t%llxh\t%f\n", (i64)packet->channel, (u64)packet->channel, (f64)packet->channel);

			//util_memory_copy(buffer + offset, packet->data, packet->data_length);
			offset += packet->data_length;

		} break;

		case Gateway_Packet_Kind_ChannelIsRoutable:
		{
			printf("[*] Packing ChannelIsRoutable...\n");
			Gateway_Packet_ChannelIsRoutable* packet = packet_ptr;

			endian_write_u8_little(buffer + offset, GATEWAY_CHANNELISROUTABLE_ID | (packet->channel << 5));
			offset++;
			printf("-- channel                 \t%lld\t%llxh\t%f\n", (i64)packet->channel, (u64)packet->channel, (f64)packet->channel);

			// b8 is_logged_in
			endian_write_b8_little(buffer + offset, packet->is_routable);
			offset += sizeof(b8);
			printf("-- is_routable             \t%lld\t%llxh\t%f\n", (i64)packet->is_routable, (u64)packet->is_routable, (f64)packet->is_routable);

			// b8 unk_bool
			endian_write_b8_little(buffer + offset, packet->unk_bool);
			offset += sizeof(b8);
			printf("-- unk_bool                \t%lld\t%llxh\t%f\n", (i64)packet->unk_bool, (u64)packet->unk_bool, (f64)packet->unk_bool);


		} break;

		default:
		{
			printf("[!] Packing %s not implemented\n", gateway_packet_names[packet_kind]);
		}
	}
	return offset;
}

internal void gateway_packet_unpack(u8* data,
                                    u32 data_length,
                                    Gateway_Packet_Kind packet_kind,
                                    void* packet_ptr,
                                    Arena* arena)
{
	u32 offset = 1;

	printf("\n");
	switch (packet_kind)
	{
		case Gateway_Packet_Kind_LoginRequest:
		{
			printf("[*] Unpacking LoginRequest...\n");
			Gateway_Packet_LoginRequest* packet = packet_ptr;

			// u64 character_id
			packet->character_id = endian_read_u64_little(data + offset);
			offset += sizeof(u64);
			printf("-- character_id            \t%lld\t%llxh\t%f\n", (i64)packet->character_id, (u64)packet->character_id, (f64)packet->character_id);

			// string server_ticket
			packet->server_ticket_length = endian_read_u32_little(data + offset);
			offset += sizeof(u32);
			packet->server_ticket = memory_arena_push_length(arena, packet->server_ticket_length);
			printf("-- STRING_LENGTH           \t%d\n", packet->server_ticket_length);
			for (u32 server_ticket_iter = 0; server_ticket_iter < packet->server_ticket_length; server_ticket_iter++)
			{
				packet->server_ticket[server_ticket_iter] = *(i8*)((u64)data + offset);
				offset++;
			}

			// string client_protocol
			packet->client_protocol_length = endian_read_u32_little(data + offset);
			offset += sizeof(u32);
			packet->client_protocol = memory_arena_push_length(arena, packet->client_protocol_length);
			printf("-- STRING_LENGTH           \t%d\n", packet->client_protocol_length);
			for (u32 client_protocol_iter = 0; client_protocol_iter < packet->client_protocol_length; client_protocol_iter++)
			{
				packet->client_protocol[client_protocol_iter] = *(i8*)((u64)data + offset);
				offset++;
			}

			// string client_build
			packet->client_build_length = endian_read_u32_little(data + offset);
			offset += sizeof(u32);
			packet->client_build = memory_arena_push_length(arena, packet->client_build_length);
			printf("-- STRING_LENGTH           \t%d\n", packet->client_build_length);
			for (u32 client_build_iter = 0; client_build_iter < packet->client_build_length; client_build_iter++)
			{
				packet->client_build[client_build_iter] = *(i8*)((u64)data + offset);
				offset++;
			}

		} break;

		case Gateway_Packet_Kind_LoginReply:
		{
			printf("[*] Unpacking LoginReply...\n");
			Gateway_Packet_LoginReply* packet = packet_ptr;

			// b8 is_logged_in
			packet->is_logged_in = endian_read_b8_little(data + offset);
			offset += sizeof(b8);
			printf("-- is_logged_in            \t%lld\t%llxh\t%f\n", (i64)packet->is_logged_in, (u64)packet->is_logged_in, (f64)packet->is_logged_in);

		} break;

		case Gateway_Packet_Kind_TunnelPacketFromExternalConnection:
		{
			printf("[*] Unpacking %s...\n", gateway_packet_names[packet_kind]);
			Gateway_Packet_TunnelPacket* packet = packet_ptr;

			packet->channel = (*data) >> 5;
			packet->data = data + offset;
			packet->data_length = data_length - 1;

		} break;

		default:
		{
			printf("[!] Unpacking %s not implemented\n", gateway_packet_names[packet_kind]);
		}
	}
}

internal void gateway_tunnel_data_send(App_State* server_state,
                                       Session_State* session_state,
                                       u8* base_buffer,
                                       u32 total_length)
{
	// NOTE(rhett): Assumes the passed buffer is prefixed with
	//              enough space for the tunnel data header
	Gateway_Packet_TunnelPacket tunnel_packet =
	{
		.channel = 0,
		.data = base_buffer + TUNNEL_DATA_HEADER_LENGTH,
		.data_length = total_length - TUNNEL_DATA_HEADER_LENGTH,
	};

	//u8* packed_buffer = memory_arena_allocate(arena, max_length);
	u32 packed_length = gateway_packet_pack(Gateway_Packet_Kind_TunnelPacketToExternalConnection,
																					&tunnel_packet,
																					base_buffer);

	// NOTE(rhett): leave room for an extra zero at the beginning
	// TODO(rhett): or not?
	//packed_buffer++;

	if (session_state->connection_args.should_dump_tunnel)
	{
		char dump_path[256] = { 0 };
		stbsp_snprintf(dump_path, 256, "packets\\%llu_%llu_S_tunneldata_%u.bin", global_tick_count, global_packet_dump_count++, tunnel_packet.channel);
		server_state->platform_api->buffer_write_to_file(dump_path, base_buffer, packed_length);
	}

	output_stream_write(&session_state->output_stream,
											base_buffer,
											packed_length,
											FALSE,
											server_state,
											session_state);
}

internal void gateway_packet_send(App_State* server_state,
                                  Session_State* session_state,
                                  Arena* arena,
                                  u32 max_length,
                                  Gateway_Packet_Kind packet_kind,
                                  void* packet_ptr)
{
	u8* packed_buffer = memory_arena_push_length(arena, max_length);
	u32 packed_length = gateway_packet_pack(packet_kind,
																					packet_ptr,
																					packed_buffer);

	// NOTE(rhett): Leave room for an extra zero at the beginning
	//packed_buffer++;

	if (session_state->connection_args.should_dump_gateway)
	{
		char dump_path[256] = { 0 };
		stbsp_snprintf(dump_path, 256, "packets\\%llu_%llu_S_gateway_%s.bin", global_tick_count, global_packet_dump_count++, gateway_packet_names[packet_kind]);
		server_state->platform_api->buffer_write_to_file(dump_path, packed_buffer, packed_length);
	}

	output_stream_write(&session_state->output_stream,
											packed_buffer,
											packed_length,
											FALSE,
											server_state,
											session_state);
}

internal void gateway_packet_handle(App_State* server_state,
																		Session_State* session_state,
																		u8* data,
																		u32 data_length)
{
	Gateway_Packet_Kind packet_kind;

	printf("\n");
	// TODO(rhett): opcode may have variable length
	u8 channel = *data >> 5;
	u8 packet_id = *data & 0b00011111;
	switch (packet_id)
	{
		case GATEWAY_LOGINREQUEST_ID:
		{
			packet_kind = Gateway_Packet_Kind_LoginRequest;
			printf("[*] (%u) Handling %s...\n", channel, gateway_packet_names[packet_kind]);

			if (session_state->connection_args.should_dump_gateway)
			{
				char dump_path[256] = { 0 };
				stbsp_snprintf(dump_path, 256, "packets\\%llu_%llu_C_gateway_%u_%s.bin", global_tick_count, global_packet_dump_count++, channel, gateway_packet_names[packet_kind]);
				server_state->platform_api->buffer_write_to_file(dump_path, data, data_length);
			}

			Gateway_Packet_LoginRequest packet = { 0 };
			gateway_packet_unpack(data, data_length, packet_kind, &packet, &server_state->arena_per_tick);

			printf("[*] Enabling encryption for session\n");
			session_state->input_stream.use_encryption = TRUE;
			session_state->output_stream.use_encryption = TRUE;

			Gateway_Packet_LoginReply login_reply =
			{
				.is_logged_in = TRUE,
			};
			gateway_packet_send(server_state, session_state, &server_state->arena_per_tick, 32, Gateway_Packet_Kind_LoginReply, &login_reply);

			Gateway_Packet_ChannelIsRoutable channel_is_routable_0 =
			{
				.channel = 0,
				.is_routable = TRUE,
			};
			gateway_packet_send(server_state, session_state, &server_state->arena_per_tick, 32, Gateway_Packet_Kind_ChannelIsRoutable, &channel_is_routable_0);

			Gateway_Packet_ChannelIsRoutable channel_is_routable_1 =
			{
				.channel = 1,
				.is_routable = TRUE,
			};
			gateway_packet_send(server_state, session_state, &server_state->arena_per_tick, 32, Gateway_Packet_Kind_ChannelIsRoutable, &channel_is_routable_1);

			gateway_on_login(server_state, session_state, packet.character_id);

		} break;

		case GATEWAY_TUNNELPACKETFROMEXTERNALCONNECTION_ID:
		{
			packet_kind = Gateway_Packet_Kind_TunnelPacketFromExternalConnection;
			printf("[*] (%u) Handling %s...\n", channel, gateway_packet_names[packet_kind]);

			if (session_state->connection_args.should_dump_gateway)
			{
				char dump_path[256] = { 0 };
				stbsp_snprintf(dump_path, 256, "packets\\%llu_%llu_C_gateway_%u_%s.bin", global_tick_count, global_packet_dump_count++, channel, gateway_packet_names[packet_kind]);
				server_state->platform_api->buffer_write_to_file(dump_path, data, data_length);
			}

			Gateway_Packet_TunnelPacket tunnel_data = { 0 };
			gateway_packet_unpack(data, data_length, Gateway_Packet_Kind_TunnelPacketFromExternalConnection, &tunnel_data, &server_state->arena_per_tick);

			if (session_state->connection_args.should_dump_tunnel)
			{
				char dump_path[256] = { 0 };
				stbsp_snprintf(dump_path, 256, "packets\\%llu_%llu_C_tunneldata_%u.bin", global_tick_count, global_packet_dump_count++, tunnel_data.channel);
				server_state->platform_api->buffer_write_to_file(dump_path, data, data_length);
			}

			gateway_on_tunnel_data_from_client(server_state, session_state, tunnel_data.data, tunnel_data.data_length);

		} break;

		default:
		{
			packet_kind = Gateway_Packet_Kind_Unhandled;
			printf("[!] (%u) Unhandled gateway packet 0x%02x\n", channel, packet_id);

			if (session_state->connection_args.should_dump_gateway)
			{
				char dump_path[256] = { 0 };
				stbsp_snprintf(dump_path, 256, "packets\\%llu_%llu_C_gateway_%u_%s.bin", global_tick_count, global_packet_dump_count++, channel, gateway_packet_names[packet_kind]);
				server_state->platform_api->buffer_write_to_file(dump_path, data, data_length);
			}
		}
	}

	
}
#endif