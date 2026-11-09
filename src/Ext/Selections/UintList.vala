public class Gth.UintList : Object {
	public UintList.from_uintv (uint[] _uintv) {
		uintv = _uintv;
	}

	public unowned uint[] get_uintv() {
		return uintv;
	}

	uint[] uintv;
}
