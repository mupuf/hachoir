#ifndef BAND_H
#define BAND_H

class Band
{
private:
	float _start;
	float _end;
public:
	Band();
	Band(float start, float end);

	float start() const { return _start; }
	float end() const { return _end; }

	bool isBandIncluded(Band b) const;
};

#endif // BAND_H
