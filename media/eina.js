$(document).ready(function() {
	// load first or target section
	curr = document.location.href;
	targetpos = curr.indexOf('#');
	if (targetpos == -1){
		load_section('about.html');
	}else{
		page = curr.substring(targetpos+1) + '.html';
		load_section(page);
	}

	// enable navigation links
	$('nav a').click(function(e){
		if (this.className == 'active'){
			return false;
		}else{
			$('nav a').removeClass('active');
			this.className = 'active';
		}
		page = this.href.substring(this.href.indexOf('#')+1) + '.html';
		return load_section(page);
	});
});

function load_section(section){
	nav = $('nav');
	$.ajax({
		url: section,
		success: function(data, status, jqXHR){
			oldsec = nav.next('section');
			oldsec.remove();
			nav.after(data);
		},
		error: function(jqXHR, status, error){
			console.log(jqXHR, status, error);
			return false;
		}
	});
}