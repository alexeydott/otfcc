#include "gasp.h"

#include "support/util.h"

#define GASP_DOGRAY 0x0002
#define GASP_GRIDFIT 0x0001
#define GASP_SYMMETRIC_GRIDFIT 0x0004
#define GASP_SYMMETRIC_SMOOTHING 0x0008

caryll_TrivialElementImpl(gasp_Record, gasp_iRecord);
caryll_DefineVectorImpl(gasp_RecordList, gasp_Record, gasp_iRecord, gasp_iRecordList);

table_gasp *otfcc_newGasp() {
	table_gasp *gasp;
	NEW(gasp);
	gasp->version = 1;
	gasp_iRecordList.init(&gasp->records);
	return gasp;
}
void otfcc_deleteGasp(table_gasp *table) {
	if (!table) return;
	gasp_iRecordList.dispose(&table->records);
	FREE(table);
}
table_gasp *otfcc_readGasp(const otfcc_Packet packet, const otfcc_Options *options) {
	table_gasp *gasp = NULL;
	FOR_TABLE('gasp', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 4) { goto FAIL; }
		gasp = otfcc_newGasp();
		gasp->version = read_16u(data);
		tableid_t numRanges = read_16u(data + 2);
		if (length < 4 + numRanges * 4) { goto FAIL; }

		for (uint32_t j = 0; j < numRanges; j++) {
			gasp_Record record;
			record.rangeMaxPPEM = read_16u(data + 4 + j * 4);
			uint16_t rangeGaspBehavior = read_16u(data + 4 + j * 4 + 2);
			record.dogray = !!(rangeGaspBehavior & GASP_DOGRAY);
			record.gridfit = !!(rangeGaspBehavior & GASP_GRIDFIT);
			record.symmetric_smoothing = !!(rangeGaspBehavior & GASP_SYMMETRIC_SMOOTHING);
			record.symmetric_gridfit = !!(rangeGaspBehavior & GASP_SYMMETRIC_GRIDFIT);
			gasp_iRecordList.push(&gasp->records, record);
		}
		return gasp;

	FAIL:
		logWarning("table 'gasp' corrupted.\n");
		otfcc_deleteGasp(gasp);
		gasp = NULL;
	}
	return NULL;
}
void otfcc_dumpGasp(const table_gasp *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("gasp") {
		json_value *t = json_array_new(table->records.length);
		for (uint16_t j = 0; j < table->records.length; j++) {
			json_value *rec = json_object_new(5);
			json_object_push(rec, "rangeMaxPPEM", json_integer_new(table->records.items[j].rangeMaxPPEM));
			json_object_push(rec, "dogray", json_boolean_new(table->records.items[j].dogray));
			json_object_push(rec, "gridfit", json_boolean_new(table->records.items[j].gridfit));
			json_object_push(rec, "symmetric_smoothing", json_boolean_new(table->records.items[j].symmetric_smoothing));
			json_object_push(rec, "symmetric_gridfit", json_boolean_new(table->records.items[j].symmetric_gridfit));
			json_array_push(t, rec);
		}
		json_object_push(root, "gasp", t);
	}
}

table_gasp *otfcc_parseGasp(const json_value *root, const otfcc_Options *options) {
	table_gasp *gasp = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "gasp", json_array))) {
		loggedStep("gasp") {
			gasp = otfcc_newGasp();
			for (uint16_t j = 0; j < table->u.array.length; j++) {
				json_value *r = table->u.array.values[j];
				if (!r || r->type != json_object) continue;
				gasp_Record record;
				record.rangeMaxPPEM = json_obj_getint_fallback(r, "rangeMaxPPEM", 0xFFFF);
				record.dogray = json_obj_getbool(r, "dogray");
				record.gridfit = json_obj_getbool(r, "gridfit");
				record.symmetric_smoothing = json_obj_getbool(r, "symmetric_smoothing");
				record.symmetric_gridfit = json_obj_getbool(r, "symmetric_gridfit");
				gasp_iRecordList.push(&gasp->records, record);
			}
		}
	}
	return gasp;
}

caryll_Buffer *otfcc_buildGasp(const table_gasp *gasp, const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!gasp) return buf;
	bufwrite16b(buf, 1);
	bufwrite16b(buf, gasp->records.length);
	for (uint16_t j = 0; j < gasp->records.length; j++) {
		gasp_Record *r = &(gasp->records.items[j]);
		bufwrite16b(buf, r->rangeMaxPPEM);
		bufwrite16b(buf, (r->dogray ? GASP_DOGRAY : 0) | (r->gridfit ? GASP_GRIDFIT : 0) |
		                     (r->symmetric_gridfit ? GASP_SYMMETRIC_GRIDFIT : 0) |
		                     (r->symmetric_smoothing ? GASP_SYMMETRIC_SMOOTHING : 0));
	}
	return buf;
}