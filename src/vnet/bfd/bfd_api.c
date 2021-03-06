/*
 *------------------------------------------------------------------
 * bfd_api.c - bfd api
 *
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */

#include <vnet/vnet.h>
#include <vlibmemory/api.h>

#include <vnet/interface.h>
#include <vnet/api_errno.h>
#include <vnet/bfd/bfd_main.h>
#include <vnet/bfd/bfd_api.h>

#include <vnet/vnet_msg_enum.h>

#define vl_typedefs		/* define message structures */
#include <vnet/vnet_all_api_h.h>
#undef vl_typedefs

#define vl_endianfun		/* define message structures */
#include <vnet/vnet_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...) vlib_cli_output (handle, __VA_ARGS__)
#define vl_printfun
#include <vnet/vnet_all_api_h.h>
#undef vl_printfun

#include <vlibapi/api_helper_macros.h>

#define foreach_vpe_api_msg                             \
_(BFD_UDP_ADD, bfd_udp_add)                             \
_(BFD_UDP_DEL, bfd_udp_del)                             \
_(BFD_UDP_SESSION_DUMP, bfd_udp_session_dump)           \
_(BFD_SESSION_SET_FLAGS, bfd_session_set_flags)         \
_(WANT_BFD_EVENTS, want_bfd_events)

pub_sub_handler (bfd_events, BFD_EVENTS);

static void
vl_api_bfd_udp_add_t_handler (vl_api_bfd_udp_add_t * mp)
{
  vl_api_bfd_udp_add_reply_t *rmp;
  int rv;

  VALIDATE_SW_IF_INDEX (mp);

  ip46_address_t local_addr;
  memset (&local_addr, 0, sizeof (local_addr));
  ip46_address_t peer_addr;
  memset (&peer_addr, 0, sizeof (peer_addr));
  if (mp->is_ipv6)
    {
      clib_memcpy (&local_addr.ip6, mp->local_addr, sizeof (local_addr.ip6));
      clib_memcpy (&peer_addr.ip6, mp->peer_addr, sizeof (peer_addr.ip6));
    }
  else
    {
      clib_memcpy (&local_addr.ip4, mp->local_addr, sizeof (local_addr.ip4));
      clib_memcpy (&peer_addr.ip4, mp->peer_addr, sizeof (peer_addr.ip4));
    }

  rv = bfd_udp_add_session (clib_net_to_host_u32 (mp->sw_if_index),
			    clib_net_to_host_u32 (mp->desired_min_tx),
			    clib_net_to_host_u32 (mp->required_min_rx),
			    mp->detect_mult, &local_addr, &peer_addr);

  BAD_SW_IF_INDEX_LABEL;
  REPLY_MACRO (VL_API_BFD_UDP_ADD_REPLY);
}

static void
vl_api_bfd_udp_del_t_handler (vl_api_bfd_udp_del_t * mp)
{
  vl_api_bfd_udp_del_reply_t *rmp;
  int rv;

  VALIDATE_SW_IF_INDEX (mp);

  ip46_address_t local_addr;
  memset (&local_addr, 0, sizeof (local_addr));
  ip46_address_t peer_addr;
  memset (&peer_addr, 0, sizeof (peer_addr));
  if (mp->is_ipv6)
    {
      clib_memcpy (&local_addr.ip6, mp->local_addr, sizeof (local_addr.ip6));
      clib_memcpy (&peer_addr.ip6, mp->peer_addr, sizeof (peer_addr.ip6));
    }
  else
    {
      clib_memcpy (&local_addr.ip4, mp->local_addr, sizeof (local_addr.ip4));
      clib_memcpy (&peer_addr.ip4, mp->peer_addr, sizeof (peer_addr.ip4));
    }

  rv =
    bfd_udp_del_session (clib_net_to_host_u32 (mp->sw_if_index), &local_addr,
			 &peer_addr);

  BAD_SW_IF_INDEX_LABEL;
  REPLY_MACRO (VL_API_BFD_UDP_DEL_REPLY);
}

void
send_bfd_udp_session_details (unix_shared_memory_queue_t * q, u32 context,
			      bfd_session_t * bs)
{
  if (bs->transport != BFD_TRANSPORT_UDP4 &&
      bs->transport != BFD_TRANSPORT_UDP6)
    {
      return;
    }

  vl_api_bfd_udp_session_details_t *mp = vl_msg_api_alloc (sizeof (*mp));
  memset (mp, 0, sizeof (*mp));
  mp->_vl_msg_id = ntohs (VL_API_BFD_UDP_SESSION_DETAILS);
  mp->context = context;
  mp->bs_index = clib_host_to_net_u32 (bs->bs_idx);
  mp->state = bs->local_state;
  bfd_udp_session_t *bus = &bs->udp;
  bfd_udp_key_t *key = &bus->key;
  mp->sw_if_index = clib_host_to_net_u32 (key->sw_if_index);
  mp->is_ipv6 = !(ip46_address_is_ip4 (&key->local_addr));
  if (mp->is_ipv6)
    {
      clib_memcpy (mp->local_addr, &key->local_addr,
		   sizeof (key->local_addr));
      clib_memcpy (mp->peer_addr, &key->peer_addr, sizeof (key->peer_addr));
    }
  else
    {
      clib_memcpy (mp->local_addr, key->local_addr.ip4.data,
		   sizeof (key->local_addr.ip4.data));
      clib_memcpy (mp->peer_addr, key->peer_addr.ip4.data,
		   sizeof (key->peer_addr.ip4.data));
    }

  vl_msg_api_send_shmem (q, (u8 *) & mp);
}

void
bfd_event (bfd_main_t * bm, bfd_session_t * bs)
{
  vpe_api_main_t *vam = &vpe_api_main;
  vpe_client_registration_t *reg;
  unix_shared_memory_queue_t *q;
  /* *INDENT-OFF* */
  pool_foreach (reg, vam->bfd_events_registrations, ({
                  q = vl_api_client_index_to_input_queue (reg->client_index);
                  if (q)
                    {
                      switch (bs->transport)
                        {
                        case BFD_TRANSPORT_UDP4:
                        /* fallthrough */
                        case BFD_TRANSPORT_UDP6:
                          send_bfd_udp_session_details (q, 0, bs);
                        }
                    }
                }));
  /* *INDENT-ON* */
}

static void
vl_api_bfd_udp_session_dump_t_handler (vl_api_bfd_udp_session_dump_t * mp)
{
  unix_shared_memory_queue_t *q;

  q = vl_api_client_index_to_input_queue (mp->client_index);

  if (q == 0)
    return;

  bfd_session_t *bs = NULL;
  /* *INDENT-OFF* */
  pool_foreach (bs, bfd_main.sessions, ({
                  if (bs->transport == BFD_TRANSPORT_UDP4 ||
                      bs->transport == BFD_TRANSPORT_UDP6)
                    send_bfd_udp_session_details (q, mp->context, bs);
                }));
  /* *INDENT-ON* */
}

static void
vl_api_bfd_session_set_flags_t_handler (vl_api_bfd_session_set_flags_t * mp)
{
  vl_api_bfd_session_set_flags_reply_t *rmp;
  int rv;

  rv =
    bfd_session_set_flags (clib_net_to_host_u32 (mp->bs_index),
			   mp->admin_up_down);

  REPLY_MACRO (VL_API_BFD_SESSION_SET_FLAGS_REPLY);
}


/*
 * bfd_api_hookup
 * Add vpe's API message handlers to the table.
 * vlib has alread mapped shared memory and
 * added the client registration handlers.
 * See .../vlib-api/vlibmemory/memclnt_vlib.c:memclnt_process()
 */
#define vl_msg_name_crc_list
#include <vnet/vnet_all_api_h.h>
#undef vl_msg_name_crc_list

static void
setup_message_id_table (api_main_t * am)
{
#define _(id,n,crc) vl_msg_api_add_msg_name_crc (am, #n "_" #crc, id);
  foreach_vl_msg_name_crc_bfd;
#undef _
}

static clib_error_t *
bfd_api_hookup (vlib_main_t * vm)
{
  api_main_t *am = &api_main;

#define _(N,n)                                                  \
    vl_msg_api_set_handlers(VL_API_##N, #n,                     \
                           vl_api_##n##_t_handler,              \
                           vl_noop_handler,                     \
                           vl_api_##n##_t_endian,               \
                           vl_api_##n##_t_print,                \
                           sizeof(vl_api_##n##_t), 1);
  foreach_vpe_api_msg;
#undef _

  /*
   * Set up the (msg_name, crc, message-id) table
   */
  setup_message_id_table (am);

  return 0;
}

VLIB_API_INIT_FUNCTION (bfd_api_hookup);

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
