import QtQuick 2.0

Canvas {
	id: canvas
	antialiasing: true

	property int margin_top: 10
	property int margin_bottom: 10
	property int margin_left: 10
	property int margin_right: 10

	property int power_range_high: -30
	property int power_range_low: -110
	property real freq_low:  868000000
	property real freq_high: 888000000

	function value_normalize(value, norm_value, toHigher)
	{
		if (value % norm_value != 0)
		{
			value = Math.floor(value / norm_value) * norm_value
			if (toHigher === 1)
				value += norm_value
		}

		return value
	}

	function biggestSize(ctx, str1, str2)
	{
		var str1_size = ctx.measureText(str1)
		var str2_size = ctx.measureText(str2)

		if (str1_size.width > str2_size.width)
			return str1_size
		else
			return str2_size;
	}

	function freqToText(freq)
	{
		var units = ["Hz", "kHz", "MHz", "GHz", "THz"]
		var unit = 0

		while (freq > 1000 && unit < 5)
		{
			freq /= 1000;
			unit++;
		}

		return freq.toFixed(2) + " " + units[unit]
	}

	function getCoordinates(ctx, freq, power)
	{
		var font_height = 10
		var dB_bar_text_size = biggestSize(ctx, canvas.power_range_low, canvas.power_range_high)
		var dB_bar_x_offset = canvas.margin_left + dB_bar_text_size.width + 5
		var freq_bar_top_y_offset = font_height
		var freq_bar_bottom_y_offset = font_height + 10	// 5 for the mark and 3 for the spacing
		var graph_width = canvas.width - dB_bar_x_offset - canvas.margin_right
		var graph_height = canvas.height - canvas.margin_top - freq_bar_top_y_offset - freq_bar_bottom_y_offset - canvas.margin_bottom
		var freq_range = canvas.freq_high - canvas.freq_low
		var power_range = canvas.power_range_low - canvas.power_range_high

		var pos = {
			x: dB_bar_x_offset + graph_width * (freq - canvas.freq_low) / freq_range,
			y: canvas.margin_top + font_height + graph_height * (power - canvas.power_range_high) / power_range,
			toString: function () {
				return "[" + this.x + "," + this.y + "]"
			}
		}

		return pos
	}

	function drawGrid(ctx)
	{
		ctx.save();

		var top_left = getCoordinates(ctx, freq_low, canvas.power_range_high)
		var bottom_left = getCoordinates(ctx, freq_low, canvas.power_range_low)
		var bottom_right = getCoordinates(ctx, freq_high, canvas.power_range_low)

		// draw the dB lines
		ctx.beginPath()
		ctx.strokeStyle = "grey"
		ctx.lineWidth = 0.25
		ctx.fillStyle = "black"
		ctx.textAlign = "right"
		ctx.textBaseline = "middle"
		for (var power = canvas.power_range_high; power >= canvas.power_range_low; power-=10)
		{
			var pos_start = getCoordinates(ctx, freq_low, power)
			var pos_end = getCoordinates(ctx, freq_high, power)

			ctx.fillText(power, pos_start.x - 5, pos_start.y)
			ctx.fill()

			ctx.moveTo(pos_start.x, pos_start.y);
			ctx.lineTo(pos_end.x, pos_end.y)
			ctx.stroke()
		}
		ctx.closePath()

		// draw the axis
		ctx.strokeStyle = "black"
		ctx.lineWidth = 2
		ctx.beginPath();
		ctx.moveTo(top_left.x, top_left.y);
		ctx.lineTo(top_left.x, bottom_right.y)
		ctx.lineTo(bottom_right.x, bottom_right.y)
		ctx.stroke()
		ctx.closePath()

		// draw the frequencies
		ctx.beginPath()
		ctx.lineWidth = 2
		ctx.textAlign = "center"
		ctx.textBaseline = "top"
		var freq_range = canvas.freq_high - canvas.freq_low
		for (var i = 0; i < 10; i++)
		{
			var freq = freq_low + i * Math.floor(freq_range / 9)
			var pos = getCoordinates(ctx, freq, canvas.power_range_low)

			//console.log(freqToText(freq_low + i * (canvas.bandwidth / 10)))
			ctx.fillText(freqToText(freq), pos.x, pos.y + 8)
			ctx.fill()

			ctx.moveTo(pos.x, pos.y - 5);
			ctx.lineTo(pos.x, pos.y + 5)
			ctx.stroke()
		}
		ctx.closePath()

		// draw the actual power state
		var spectrum = [[868000000, -90],
						[869000000, -83],
						[870000000, -84],
						[871000000, -86],
						[872000000, -70],
						[873000000, -40],
						[874000000, -45],
						[875000000, -82],
						[876000000, -93],
						[877000000, -30],
						[878000000, -60],
						[879000000, -100],
						[880000000, -83],
						[881000000, -89],
						[882000000, -91],
						[883000000, -89],
						[884000000, -90],
						[885000000, -70],
						[886000000, -36],
						[887000000, -50],
						[888000000, -90]]

		var gradientFill = ctx.createLinearGradient(top_left.x, top_left.y, bottom_left.x, bottom_left.y)
		gradientFill.addColorStop(0, Qt.rgba(1, 0, 0, 0.3));
		gradientFill.addColorStop(0.5, Qt.rgba(0, 0.8, 0, 0.3));
		gradientFill.addColorStop(1, Qt.rgba(0, 0, 0.8, 0.3));

		ctx.strokeStyle = "black"
		ctx.fillStyle = gradientFill
		ctx.lineWidth = 1.5
		ctx.beginPath()
		var size = SensingNode.startReading()
		for (var i = 0; i < size; i++)
		{
			var sample = SensingNode.getSample(i)
			pos = getCoordinates(ctx, sample["freq"], sample["dbm"])

			if (i == 0)
				ctx.moveTo(pos.x, bottom_left.y)
			ctx.lineTo(pos.x, pos.y)
		}
		SensingNode.stopReading()
		ctx.lineTo(pos.x, bottom_right.y)
		ctx.closePath()
		ctx.stroke()
		ctx.fill()

		ctx.restore()
	}

	Connections {
		target: SensingNode
		onPowerRangeChanged: {
			power_range_high = high
			power_range_low = low
		}
		onFrequencyRangeChanged: {
			freq_high = high
			freq_low = low
		}
	}

	onCanvasSizeChanged: requestPaint()
	onCanvasWindowChanged: requestPaint()

	onPaint: {
		var ctx = canvas.getContext('2d');
		ctx.save();

		// erase everything
		ctx.fillStyle = "white";
		ctx.fillRect(0, 0, canvas.width, canvas.height);

		drawGrid(ctx)
		ctx.restore();
	}
}
