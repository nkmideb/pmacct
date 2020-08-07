/*  
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2020 by Paolo Lucente
*/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* includes */
#include "pmacct.h"
#include "addr.h"
#include "bgp/bgp.h"
#include "bmp.h"
#if defined WITH_RABBITMQ
#include "amqp_common.h"
#endif
#ifdef WITH_KAFKA
#include "kafka_common.h"
#endif
#ifdef WITH_AVRO
#include "plugin_cmn_avro.h"
#endif

char *bmp_get_and_check_length(char **bmp_packet_ptr, u_int32_t *pkt_size, u_int32_t len)
{
  char *current_ptr = NULL;
  
  if (bmp_packet_ptr && (*bmp_packet_ptr) && pkt_size) {
    if ((*pkt_size) >= len) {
      current_ptr = (*bmp_packet_ptr);
      (*pkt_size) -= len;
      (*bmp_packet_ptr) += len;
    }
  }

  return current_ptr;
}

void bmp_jump_offset(char **bmp_packet_ptr, u_int32_t *len, u_int32_t offset)
{
  if (bmp_packet_ptr && (*bmp_packet_ptr) && len) {
    if (offset <= (*len)) {
      (*bmp_packet_ptr) += offset;
      (*len) -= offset;
    }
  }
}

u_int32_t bmp_packet_adj_offset(char *bmp_packet, u_int32_t buf_len, u_int32_t recv_len, u_int32_t remaining_len, char *addr_str)
{
  char tmp_packet[BGP_BUFFER_SIZE];
  
  if (!bmp_packet || recv_len > buf_len || remaining_len >= buf_len || remaining_len > recv_len) {
    if (addr_str)
      Log(LOG_INFO, "INFO ( %s/core/BMP ): [%s] packet discarded: failed bmp_packet_adj_offset()\n", config.name, addr_str);

    return FALSE;
  }

  memcpy(tmp_packet, &bmp_packet[recv_len - remaining_len], remaining_len);
  memcpy(bmp_packet, tmp_packet, remaining_len);

  return remaining_len;
}

void bgp_peer_log_msg_extras_bmp(struct bgp_peer *peer, int output, void *void_obj)
{
  struct bgp_misc_structs *bms;
  struct bmp_peer *bmpp;

  if (!peer || !void_obj) return;

  bms = bgp_select_misc_db(peer->type);
  bmpp = peer->bmp_se;
  if (!bms || !bmpp) return;

  if (output == PRINT_OUTPUT_JSON) {
#ifdef WITH_JANSSON
    char bmp_msg_type[] = "route_monitor";
    char ip_address[INET6_ADDRSTRLEN];
    json_t *obj = void_obj;

    addr_to_str(ip_address, &bmpp->self.addr);
    json_object_set_new_nocheck(obj, "bmp_router", json_string(ip_address));

    json_object_set_new_nocheck(obj, "bmp_router_port", json_integer((json_int_t)bmpp->self.tcp_port));

    json_object_set_new_nocheck(obj, "bmp_msg_type", json_string(bmp_msg_type));
#endif
  }
  else if (output == PRINT_OUTPUT_AVRO_BIN) {
#ifdef WITH_AVRO
    char bmp_msg_type[] = "route_monitor";
    char ip_address[INET6_ADDRSTRLEN];
    avro_value_t *obj = (avro_value_t *) void_obj, p_avro_field;

    addr_to_str(ip_address, &bmpp->self.addr);
    pm_avro_check(avro_value_get_by_name(obj, "bmp_router", &p_avro_field, NULL));
    pm_avro_check(avro_value_set_string(&p_avro_field, ip_address));

    pm_avro_check(avro_value_get_by_name(obj, "bmp_router_port", &p_avro_field, NULL));
    pm_avro_check(avro_value_set_int(&p_avro_field, bmpp->self.tcp_port)); 

    pm_avro_check(avro_value_get_by_name(obj, "bmp_msg_type", &p_avro_field, NULL));
    pm_avro_check(avro_value_set_string(&p_avro_field, bmp_msg_type));
#endif
  }
}

void bmp_link_misc_structs(struct bgp_misc_structs *bms)
{
#if defined WITH_RABBITMQ
  bms->msglog_amqp_host = &bmp_daemon_msglog_amqp_host;
#endif
#if defined WITH_KAFKA
  bms->msglog_kafka_host = &bmp_daemon_msglog_kafka_host;
#endif
  bms->max_peers = config.bmp_daemon_max_peers;
  bms->peers = bmp_peers;
  bms->peers_cache = NULL;
  bms->peers_port_cache = NULL;
  bms->xconnects = NULL;
  bms->dump_file = config.bmp_dump_file;
  bms->dump_amqp_routing_key = config.bmp_dump_amqp_routing_key;
  bms->dump_amqp_routing_key_rr = config.bmp_dump_amqp_routing_key_rr;
  bms->dump_kafka_topic = config.bmp_dump_kafka_topic;
  bms->dump_kafka_topic_rr = config.bmp_dump_kafka_topic_rr;
  bms->dump_kafka_avro_schema_registry = config.bmp_dump_kafka_avro_schema_registry;
  bms->msglog_file = config.bmp_daemon_msglog_file;
  bms->msglog_output = config.bmp_daemon_msglog_output;
  bms->msglog_amqp_routing_key = config.bmp_daemon_msglog_amqp_routing_key;
  bms->msglog_amqp_routing_key_rr = config.bmp_daemon_msglog_amqp_routing_key_rr;
  bms->msglog_kafka_topic = config.bmp_daemon_msglog_kafka_topic;
  bms->msglog_kafka_topic_rr = config.bmp_daemon_msglog_kafka_topic_rr;
  bms->msglog_kafka_avro_schema_registry = config.bmp_daemon_msglog_kafka_avro_schema_registry;
  bms->peer_str = malloc(strlen("bmp_router") + 1);
  strcpy(bms->peer_str, "bmp_router");
  bms->peer_port_str = malloc(strlen("bmp_router_port") + 1);
  strcpy(bms->peer_port_str, "bmp_router_port");
  bms->bgp_peer_log_msg_extras = bgp_peer_log_msg_extras_bmp;
  bms->bgp_peer_logdump_initclose_extras = NULL;

  bms->bgp_peer_logdump_extra_data = bgp_extra_data_print_bmp;
  bms->bgp_extra_data_process = bgp_extra_data_process_bmp;
  bms->bgp_extra_data_cmp = bgp_extra_data_cmp_bmp;
  bms->bgp_extra_data_free = bgp_extra_data_free_bmp;

  bms->table_peer_buckets = config.bmp_table_peer_buckets;
  bms->table_per_peer_buckets = config.bmp_table_per_peer_buckets;
  bms->table_attr_hash_buckets = config.bmp_table_attr_hash_buckets;
  bms->table_per_peer_hash = config.bmp_table_per_peer_hash;
  bms->route_info_modulo = bmp_route_info_modulo;
  bms->bgp_lookup_find_peer = bgp_lookup_find_bmp_peer;
  bms->bgp_lookup_node_match_cmp = bgp_lookup_node_match_cmp_bmp;

  bms->bgp_msg_open_router_id_check = NULL;

  if (!bms->is_thread && !bms->dump_backend_methods) bms->skip_rib = TRUE;
}

struct bgp_peer *bmp_sync_loc_rem_peers(struct bgp_peer *bgp_peer_loc, struct bgp_peer *bgp_peer_rem)
{
  if (!bgp_peer_loc || !bgp_peer_rem) return NULL;

  if (!bgp_peer_loc->cap_4as || !bgp_peer_rem->cap_4as) bgp_peer_rem->cap_4as = FALSE;

  /* XXX: since BGP OPENs are fabricated, we assume that if remote
     peer is marked as able to send ADD-PATH capability, the local
     pper will be able to receive it just fine */
  /* if (!bgp_peer_loc->cap_add_paths || !bgp_peer_rem->cap_add_paths) bgp_peer_rem->cap_add_paths = FALSE; */

  bgp_peer_rem->type = FUNC_TYPE_BMP;
  memcpy(&bgp_peer_rem->id, &bgp_peer_rem->addr, sizeof(struct host_addr));

  return bgp_peer_rem;
}

int bmp_peer_init(struct bmp_peer *bmpp, int type)
{
  int ret;

  if (!bmpp) return ERR;

  ret = bgp_peer_init(&bmpp->self, type);
  log_notification_init(&bmpp->missing_peer_up);

  return ret;
}

void bmp_peer_close(struct bmp_peer *bmpp, int type)
{
  struct bgp_misc_structs *bms;
  struct bgp_peer *peer;

  if (!bmpp) return;

  peer = &bmpp->self;
  bms = bgp_select_misc_db(peer->type);

  if (!bms) return;

  pm_twalk(bmpp->bgp_peers_v4, bgp_peers_bintree_walk_delete, NULL);
  pm_twalk(bmpp->bgp_peers_v6, bgp_peers_bintree_walk_delete, NULL);

  pm_tdestroy(&bmpp->bgp_peers_v4, bgp_peer_free);
  pm_tdestroy(&bmpp->bgp_peers_v6, bgp_peer_free);

  if (bms->dump_file || bms->dump_amqp_routing_key || bms->dump_kafka_topic)
    bmp_dump_close_peer(peer);

  bgp_peer_close(peer, type, FALSE, FALSE, FALSE, FALSE, NULL);
}

void bgp_msg_data_set_data_bmp(struct bmp_chars *bmed_bmp, struct bmp_data *bdata)
{
  memcpy(bmed_bmp, &bdata->chars, sizeof(struct bmp_chars));
}

int bgp_extra_data_cmp_bmp(struct bgp_msg_extra_data *a, struct bgp_msg_extra_data *b) 
{
  if (a->id == b->id && a->len == b->len && a->id == BGP_MSG_EXTRA_DATA_BMP) {
    struct bmp_chars *bca = a->data;
    struct bmp_chars *bcb = b->data;

    if (bca->peer_type == bcb->peer_type &&
	bca->is_post == bcb->is_post &&
	bca->is_filtered == bcb->is_filtered &&
	bca->is_out == bcb->is_out &&
        bca->is_loc == bcb->is_loc &&
	!memcmp(&bca->rd, &bcb->rd, sizeof(bca->rd))) {
      return FALSE;
    }
    else {
      return TRUE;
    }
  }
  else {
    return ERR;
  }
}

int bgp_extra_data_process_bmp(struct bgp_msg_extra_data *bmed, struct bgp_info *ri)
{
  struct bgp_info_extra *rie = NULL;
  struct bmp_chars *bmed_bmp_src = NULL, *bmed_bmp_dst = NULL;
  int ret = BGP_MSG_EXTRA_DATA_NONE;

  if (bmed && ri && bmed->id == BGP_MSG_EXTRA_DATA_BMP) {
    rie = bgp_info_extra_get(ri);
    if (rie) {
      if (rie->bmed.data && (rie->bmed.len != bmed->len)) {
	free(rie->bmed.data);
	rie->bmed.data = NULL;
      }

      if (!rie->bmed.data) rie->bmed.data = malloc(bmed->len);

      if (rie->bmed.data) {
	memcpy(rie->bmed.data, bmed->data, bmed->len);
	rie->bmed.len = bmed->len; 
	rie->bmed.id = bmed->id;

	bmed_bmp_src = (struct bmp_chars *) bmed->data;
	bmed_bmp_dst = (struct bmp_chars *) rie->bmed.data;

	if (bmed_bmp_src->tlvs) {
	  bmed_bmp_dst->tlvs = bmp_tlv_list_copy(bmed_bmp_src->tlvs);
	}

	ret = BGP_MSG_EXTRA_DATA_BMP;	
      }
    }
  }

  return ret;
}

void bgp_extra_data_free_bmp(struct bgp_msg_extra_data *bmed)
{
  struct bmp_chars *bmed_bmp;

  if (bmed && bmed->id == BGP_MSG_EXTRA_DATA_BMP) {
    if (bmed->data) {
      bmed_bmp = (struct bmp_chars *) bmed->data;

      if (bmed_bmp->tlvs) {
	bmp_tlv_list_destroy(bmed_bmp->tlvs);
      }

      free(bmed->data);
    }

    memset(bmed, 0, sizeof(struct bgp_msg_extra_data));
  }
}

void bgp_extra_data_print_bmp(struct bgp_msg_extra_data *bmed, int output, void *void_obj)
{
  struct bmp_chars *bmed_bmp;

  if (!bmed || !void_obj || bmed->id != BGP_MSG_EXTRA_DATA_BMP) return;

  bmed_bmp = bmed->data;

  if (output == PRINT_OUTPUT_JSON) {
#ifdef WITH_JANSSON
    json_t *obj = void_obj;

    if (bmed_bmp->is_loc) {
      json_object_set_new_nocheck(obj, "is_filtered", json_integer((json_int_t)bmed_bmp->is_filtered));
      json_object_set_new_nocheck(obj, "is_loc", json_integer((json_int_t)bmed_bmp->is_loc));
    }
    else if (bmed_bmp->is_out) {
      json_object_set_new_nocheck(obj, "is_post", json_integer((json_int_t)bmed_bmp->is_post));
      json_object_set_new_nocheck(obj, "is_out", json_integer((json_int_t)bmed_bmp->is_out));
    }
#endif
  }
  else if (output == PRINT_OUTPUT_AVRO_BIN) {
#ifdef WITH_AVRO
    avro_value_t *obj = (avro_value_t *) void_obj, p_avro_field, p_avro_branch;

    if (bmed_bmp->is_loc) {
      pm_avro_check(avro_value_get_by_name(obj, "is_filtered", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, TRUE, &p_avro_branch));
      pm_avro_check(avro_value_set_int(&p_avro_branch, bmed_bmp->is_filtered));

      pm_avro_check(avro_value_get_by_name(obj, "is_loc", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, TRUE, &p_avro_branch));
      pm_avro_check(avro_value_set_int(&p_avro_branch, bmed_bmp->is_loc));

      pm_avro_check(avro_value_get_by_name(obj, "is_post", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));

      pm_avro_check(avro_value_get_by_name(obj, "is_out", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));
    }
    else if (bmed_bmp->is_out) {
      pm_avro_check(avro_value_get_by_name(obj, "is_filtered", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));

      pm_avro_check(avro_value_get_by_name(obj, "is_loc", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));

      pm_avro_check(avro_value_get_by_name(obj, "is_post", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, TRUE, &p_avro_branch));
      pm_avro_check(avro_value_set_int(&p_avro_branch, bmed_bmp->is_post));

      pm_avro_check(avro_value_get_by_name(obj, "is_out", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, TRUE, &p_avro_branch));
      pm_avro_check(avro_value_set_int(&p_avro_branch, bmed_bmp->is_out));
    }
    else {
      pm_avro_check(avro_value_get_by_name(obj, "is_filtered", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));

      pm_avro_check(avro_value_get_by_name(obj, "is_loc", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));

      pm_avro_check(avro_value_get_by_name(obj, "is_post", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));

      pm_avro_check(avro_value_get_by_name(obj, "is_out", &p_avro_field, NULL));
      pm_avro_check(avro_value_set_branch(&p_avro_field, FALSE, &p_avro_branch));
    }
#endif
  }

  if (bmed_bmp->tlvs) {
    bmp_log_msg_route_monitor_tlv(bmed_bmp->tlvs, output, void_obj);
  }
}

char *bmp_term_reason_print(u_int16_t in)
{
  char *out = NULL;
  int value_len;

  if (in <= BMP_TERM_REASON_MAX) {
    value_len = strlen(bmp_term_reason_types[in]);
    out = malloc(value_len + 1 /* null */);
    sprintf(out, "%s", bmp_term_reason_types[in]);
  }
  else {
    out = malloc(5 /* value len */ + 1 /* null */);
    sprintf(out, "%u", in);
  }

  return out;
}
