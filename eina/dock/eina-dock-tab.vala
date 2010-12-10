namespace Eina
{
	public class DockTab : GLib.Object
	{
		public string     id      { get; private set; }
		public Gtk.Widget widget  { get; private set; }
		public Gtk.Widget label   { get; private set; }
		public bool       primary { get; set; }

		public DockTab(string id, Gtk.Widget widget, Gtk.Widget label, bool primary)
			requires((id != null) && (id != "") && (widget is Gtk.Widget) && (label is Gtk.Widget))
		{
			this.id      = id;
			this.widget  = widget;
			this.label   = label;
			this.primary = primary;
		}

		public bool equal(DockTab b)
			requires ((this is DockTab) && (b is DockTab))
		{
			return (this.id == b.id) && (this.widget == b.widget);
		}
	}
}
