#include "legacy.hpp"

#include "../debug.hpp"

#include <stdexcept>
#include <string>

namespace aoe {
namespace io {

DrsBkg DRS::open_bkg(DrsId k) {
	ZoneScoped;
	uint32_t id = (uint32_t)k;
	DrsItem key{ id, 0, 0 };
	auto it = items.find(key);
	if (it == items.end())
		throw std::runtime_error(std::string("invalid background ID: ") + std::to_string(id));

	DrsBkg bkg{ 0 };
	const DrsItem &item = *it;

	// fetch data
	in.seekg(item.offset);
	std::string data(item.size, ' ');
	in.read(data.data(), item.size);

	if (sscanf(data.data(),
			"background1_files %15s none %d -1\n"
			"background2_files %15s none %d -1\n"
			"background3_files %15s none %d -1\n"
			"palette_file %15s %u\n"
			"cursor_file %15s %u\n"
			"shade_amount percent %u\n"
			"button_file %s %d\n"
			"popup_dialog_sin %15s %u\n"
			"background_position %u\n"
			"background_color %u\n"
			"bevel_colors %u %u %u %u %u %u\n"
			"text_color1 %u %u %u\n"
			"text_color2 %u %u %u\n"
			"focus_color1 %u %u %u\n"
			"focus_color2 %u %u %u\n"
			"state_color1 %u %u %u\n"
			"state_color2 %u %u %u\n",
			bkg.bkg_name1, &bkg.bkg_id[0],
			bkg.bkg_name2, &bkg.bkg_id[1],
			bkg.bkg_name3, &bkg.bkg_id[2],
			bkg.pal_name, &bkg.pal_id,
			bkg.cur_name, &bkg.cur_id,
			&bkg.shade,
			bkg.btn_file, &bkg.btn_id,
			bkg.dlg_name, &bkg.dlg_id,
			&bkg.bkg_pos, &bkg.bkg_col,
			&bkg.bevel_col[0], &bkg.bevel_col[1], &bkg.bevel_col[2],
			&bkg.bevel_col[3], &bkg.bevel_col[4], &bkg.bevel_col[5],
			&bkg.text_col[0], &bkg.text_col[1], &bkg.text_col[2],
			&bkg.text_col[3], &bkg.text_col[4], &bkg.text_col[5],
			&bkg.focus_col[0], &bkg.focus_col[1], &bkg.focus_col[2],
			&bkg.focus_col[3], &bkg.focus_col[4], &bkg.focus_col[5],
			&bkg.state_col[0], &bkg.state_col[1], &bkg.state_col[2],
			&bkg.state_col[3], &bkg.state_col[4], &bkg.state_col[5]) != 6 + 11 + 4 * 6)
		throw std::runtime_error(std::string("Could not load background ") + std::to_string(id) + ": bad data");

	if (bkg.bkg_id[1] < 0)
		bkg.bkg_id[1] = bkg.bkg_id[0];
	if (bkg.bkg_id[2] < 0)
		bkg.bkg_id[2] = bkg.bkg_id[0];

	return bkg;
}

}

}
