/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#include "cominc.h"
#include "callg.h"

//
//START CALLG
//
void CALLG::compute_entry_list(LIST<CALL_NODE*> & elst)
{
	elst.clean();
	INT c;
	for (VERTEX * v = get_first_vertex(c);
		 v != NULL; v = get_next_vertex(c)) {
		if (VERTEX_in_list(v) == NULL) {
			CALL_NODE * cn = m_cnid2cn_map.get(VERTEX_id(v));
			IS_TRUE0(cn != NULL);
			elst.append_tail(cn);
		}
	}
}


void CALLG::compute_exit_list(LIST<CALL_NODE*> & elst)
{
	elst.clean();
	INT c;
	for (VERTEX * v = get_first_vertex(c);
		 v != NULL; v = get_next_vertex(c)) {
		if (VERTEX_out_list(v) == NULL) {
			CALL_NODE * cn = m_cnid2cn_map.get(VERTEX_id(v));
			IS_TRUE0(cn != NULL);
			elst.append_tail(cn);
		}
	}
}


void CALLG::dump_vcg(DT_MGR * dm, CHAR const* name, INT flag)
{
	if (name == NULL) {
		name = "graph_call_graph.vcg";
	}
	unlink(name);
	FILE * h = fopen(name, "a+");
	IS_TRUE(h != NULL, ("%s create failed!!!",name));

	//Print comment
	fprintf(h, "\n/*");
	FILE * old = g_tfile;
	g_tfile = h;
	//....
	g_tfile = old;
	fprintf(h, "\n*/\n");

	//Print graph structure description.
	fprintf(h, "graph: {"
			  "title: \"GRAPH\"\n"
			  "shrink:  15\n"
			  "stretch: 27\n"
			  "layout_downfactor: 1\n"
			  "layout_upfactor: 1\n"
			  "layout_nearfactor: 1\n"
			  "layout_splinefactor: 70\n"
			  "spreadlevel: 1\n"
			  "treefactor: 0.500000\n"
			  "node_alignment: center\n"
			  "orientation: top_to_bottom\n"
			  "late_edge_labels: no\n"
			  "display_edge_labels: yes\n"
			  "dirty_edge_labels: no\n"
			  "finetuning: no\n"
			  "nearedges: no\n"
			  "splines: yes\n"
			  "ignoresingles: no\n"
			  "straight_phase: no\n"
			  "priority_phase: no\n"
			  "manhatten_edges: no\n"
			  "smanhatten_edges: no\n"
			  "port_sharing: no\n"
			  "crossingphase2: yes\n"
			  "crossingoptimization: yes\n"
			  "crossingweight: bary\n"
			  "arrow_mode: free\n"
			  "layoutalgorithm: mindepthslow\n"
			  "node.borderwidth: 2\n"
			  "node.color: lightcyan\n"
			  "node.textcolor: black\n"
			  "node.bordercolor: blue\n"
			  "edge.color: darkgreen\n");

	//Print node
	old = g_tfile;
	g_tfile = h;
	INT c;
	for (VERTEX * v = m_vertices.get_first(c);
		 v != NULL; v = m_vertices.get_next(c)) {
		INT id = VERTEX_id(v);
		CALL_NODE * cn = m_cnid2cn_map.get(id);
		IS_TRUE0(cn != NULL);
		fprintf(h, "\nnode: { title:\"%d\" shape:box color:gold "
				   "fontname:\"courB\" label:\"", id);
		CHAR const* ss = SYM_name(CN_sym(cn));
		fprintf(h, "FUNC(%d):%s\n", CN_id(cn), SYM_name(CN_sym(cn)));

		//
		fprintf(h, "\n");
		if (HAVE_FLAG(flag, CALLG_DUMP_IR) && CN_ru(cn) != NULL) {
			g_indent = 0;
			IR * irs = CN_ru(cn)->get_ir_list();
			for (; irs != NULL; irs = IR_next(irs)) {
				//fprintf(h, "%s\n", dump_ir_buf(ir, buf));
				//TODO: implement dump_ir_buf();
				dump_ir(irs, m_dm, NULL, true, false);
			}
		}
		//

		fprintf(h, "\"}");
	}

	//Print edge
	for (EDGE * e = m_edges.get_first(c); e != NULL; e = m_edges.get_next(c)) {
		VERTEX * from = EDGE_from(e);
		VERTEX * to = EDGE_to(e);
		fprintf(h, "\nedge: { sourcename:\"%d\" targetname:\"%d\" %s}",
				VERTEX_id(from), VERTEX_id(to),  "");
	}
	g_tfile = old;
	fprintf(h, "\n}\n");
	fclose(h);

}


/*
Insure CALL_NODE for function is unique.
Do NOT modify ir' content.
*/
CALL_NODE * CALLG::new_call_node(IR * ir)
{
	IS_TRUE0(IR_type(ir) == IR_CALL);
	SYM * name = VAR_name(CALL_idinfo(ir));
	CALL_NODE * cn  = m_sym2cn_map.get(name);
	if (cn != NULL) return cn;

	cn = new_call_node();
	CN_sym(cn) = name;
	CN_id(cn) = m_cn_count++;
	m_sym2cn_map.set(name, cn);
	return cn;
}


//To guarantee CALL_NODE of function is unique.
CALL_NODE * CALLG::new_call_node(REGION * ru)
{
	IS_TRUE0(RU_type(ru) == RU_FUNC);
	SYM * name = VAR_name(ru->get_ru_var());
	CALL_NODE * cn = m_sym2cn_map.get(name);
	if (cn != NULL) {
		if (CN_ru(cn) == NULL) {
			CN_ru(cn) = ru;
			m_ruid2cn_map.set(RU_id(ru), cn);
		}
		IS_TRUE(CN_ru(cn) == ru, ("more than 2 ru with same name"));
		return cn;
	}

	cn = new_call_node();
	CN_sym(cn) = name;
	CN_id(cn) = m_cn_count++;
	CN_ru(cn) = ru;
	m_sym2cn_map.set(name, cn);
	m_ruid2cn_map.set(RU_id(ru), cn);
	return cn;
}


//Build call graph.
void CALLG::build(REGION * top)
{
	IS_TRUE0(RU_type(top) == RU_PROGRAM);
	IR * irs = top->get_ir_list();
	LIST<CALL_NODE*> ic;
	while (irs != NULL) {
		if (IR_type(irs) == IR_REGION) {
			REGION * ru = REGION_ru(irs);
			IS_TRUE0(ru && RU_type(ru) == RU_FUNC);
			CALL_NODE * cn = new_call_node(ru);
			add_node(cn);
			LIST<IR const*> * call_list = ru->get_call_list();
			if (call_list->get_elem_count() == 0) {
				irs = IR_next(irs);
				continue;
			}
			for (IR const* ir = call_list->get_head();
				 ir != NULL; ir = call_list->get_next()) {
				IS_TRUE0(IR_type(ir) == IR_CALL || IR_type(ir) == IR_ICALL);
				if (IR_type(ir) == IR_CALL) {
					//Direct call.
					CALL_NODE * cn2 = new_call_node(const_cast<IR*>(ir));
					add_node(cn2);
					add_edge(CN_id(cn), CN_id(cn2));
				} else {
					//Indirect call.
					CALL_NODE * cn3 = new_call_node(const_cast<IR*>(ir));
					ic.append_tail(cn3);
					add_node(cn3);
				}
			}
		}
		irs = IR_next(irs);
	}
	for (CALL_NODE * i = ic.get_head(); i != NULL; i = ic.get_next()) {
		INT c;
		for (VERTEX * v = get_first_vertex(c);
			 v != NULL; v = get_next_vertex(c)) {
			CALL_NODE * j = map_id2cn(VERTEX_id(v));
			IS_TRUE0(j);
			add_edge(CN_id(i), CN_id(j));
		}
	}
	//dump_vcg(m_ru_mgr->get_dt_mgr());
	set_bs_mgr(m_ru_mgr->get_bs_mgr());
}
//END CALLG
