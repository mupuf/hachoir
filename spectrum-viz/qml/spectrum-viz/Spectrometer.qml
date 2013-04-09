import QtQuick 2.0

Canvas {
	id: canvas
	antialiasing: true
	renderStrategy: Canvas.Immediate

	property int clientID: -1

	property int power_range_high: 40
	property int power_range_low: -10
	property real freq_low:  0
	property real freq_high: 32000
	property real time: 0

	property int margin_top: 10
	property int margin_bottom: 10
	property int margin_left: 10
	property int margin_right: 10
	property int i:0

	Component.onCompleted: {
	}

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

	function getCoordinatesPreCache(ctx)
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

		var cache = {
			font_height: font_height,
			dB_bar_x_offset: dB_bar_x_offset,
			graph_width: graph_width,
			graph_height: graph_height,
			freq_range: freq_range,
			power_range: power_range,
			x_offset: dB_bar_x_offset,
			y_offset: canvas.margin_top + font_height,
			x_factor: graph_width / freq_range,
			y_factor: graph_height / power_range
		}

		return cache;
	}

	function getCoordinates(ctx, cache, freq, power)
	{
		var pos = {
			x: cache.x_offset + cache.x_factor * (freq - canvas.freq_low),
			y: cache.y_offset + cache.y_factor * (power - canvas.power_range_high),
			toString: function () {
				return "[" + this.x + "," + this.y + "]"
			}
		}

		return pos
	}

	function drawGrid(ctx)
	{
		ctx.save();

		var SensingNode = SensingServer.sensingNode(clientID)
		SensingNode.fetchEntries(-1, 0);
		var pos = SensingNode.getEntriesRange()["start"];
		var spectrum = SensingNode.selectEntry(pos);

		time = spectrum.timeNs();

		var gc_cache = getCoordinatesPreCache(ctx)

		var top_left = getCoordinates(ctx, gc_cache, freq_low, canvas.power_range_high)
		var bottom_left = getCoordinates(ctx, gc_cache, freq_low, canvas.power_range_low)
		var bottom_right = getCoordinates(ctx, gc_cache, freq_high, canvas.power_range_low)

		// draw the dB lines
		ctx.beginPath()
		ctx.strokeStyle = "grey"
		ctx.lineWidth = 0.25
		ctx.fillStyle = "black"
		ctx.textAlign = "right"
		ctx.textBaseline = "middle"
		for (var power = canvas.power_range_high; power >= canvas.power_range_low; power-=10)
		{
			var pos_start = getCoordinates(ctx, gc_cache, freq_low, power)
			var pos_end = getCoordinates(ctx, gc_cache, freq_high, power)

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
			var pos = getCoordinates(ctx, gc_cache, freq, canvas.power_range_low)

			//console.log(freqToText(freq_low + i * (canvas.bandwidth / 10)))
			ctx.fillText(freqToText(freq), pos.x, pos.y + 8)
			ctx.fill()

			ctx.moveTo(pos.x, pos.y - 5);
			ctx.lineTo(pos.x, pos.y + 5)
			ctx.stroke()
		}
		ctx.closePath()

		// draw the actual power state
		var gradientFill = ctx.createLinearGradient(top_left.x, top_left.y, bottom_left.x, bottom_left.y)
		gradientFill.addColorStop(0, Qt.rgba(1, 0, 0, 0.3));
		gradientFill.addColorStop(0.5, Qt.rgba(0, 0.8, 0, 0.3));
		gradientFill.addColorStop(1, Qt.rgba(0, 0, 0.8, 0.3));
		ctx.strokeStyle = "black"
		ctx.fillStyle = gradientFill
		ctx.lineWidth = 1.5
		ctx.beginPath()
		var size = spectrum.samplesCount();
		for (var i = 0; i < size; i++)
		{
			var freq = spectrum.sampleFrequency(i)
			var pwr = spectrum.sampleDbm(i)
			pos = getCoordinates(ctx, gc_cache, freq, pwr)

			if (i == 0)
				ctx.moveTo(pos.x, bottom_left.y)
			ctx.lineTo(pos.x, pos.y)
		}
		SensingNode.freeEntries()
		ctx.lineTo(pos.x, bottom_right.y)
		ctx.closePath()
		ctx.stroke()
		ctx.fill()

		// set the current time
		ctx.fillStyle = "black"
		ctx.fillText(time + "ns", parent.width - 100, 3)

		ctx.restore()
	}

	Connections {
		target: SensingServer.sensingNode(clientID)
		onDataChanged: {
			requestPaint()
		}
		onPowerRangeChanged: {
			power_range_high = high
			power_range_low = low
		}
		onFrequencyRangeChanged: {
			freq_high = high
			freq_low = low
		}
		onTimeChanged: {
			time = timeNs
		}
		onRequestDestroy: {
			canvas.destroy()
		}
	}

	/* refresh at around 60 fps */
	Timer {
			interval: 16; running: true; repeat: true;
			onTriggered: requestPaint();
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

		/*if (i < 1024) {
			save("/tmp/heya_" + i + ".png");
			i++;
		}*/
	}
}
