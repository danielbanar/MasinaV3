#!/usr/bin/haserl
<%in p/common.cgi %>

<%
page_title="Config File Editor & Wireguard Settings"
config_file="/root/config.txt"

if [ "$REQUEST_METHOD" = "POST" ]; then
    case "$POST_action" in
        editconfig)
            NEW_CONTENT="$POST_content"
            if printf "%s" "$NEW_CONTENT" > "$config_file"; then
                redirect_back "success" "Config file updated successfully"
            else
                redirect_back "error" "Failed to write config file"
            fi
            exit
            ;;
        reload)
            output=$(wg setconf wg0 /etc/wireguard.conf 2>&1)
            if [ $? -eq 0 ]; then
                redirect_back "success" "Wireguard configuration has been reloaded."
            else
                redirect_back "danger" "Failed to reload Wireguard configuration: $output"
            fi
            ;;
    esac
    exit
fi
%>

<%in p/header.cgi %>

<div class="row g-4">
    <div class="col-12 col-md-6">
        <!-- Config Editor -->
        <form action="<%= $SCRIPT_NAME %>" method="post">
            <% field_hidden "action" "editconfig" %>
            <div class="mb-3">
                <label class="form-label">Edit config.txt:</label>
                <textarea class="form-control font-monospace w-100" 
                         name="content" 
                         rows="20"
                         style="width: 100%"><%
                    if [ -f "$config_file" ]; then
                        cat "$config_file" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g'
                    else
                        echo "Config file not found!"
                    fi
                %></textarea>
            </div>
            <% button_submit "Save Changes" "primary" %>
        </form>
    </div>

    <div class="col-12 col-md-6">
        <!-- File displays -->
        <div class="card border-0 mb-4">
            <div class="card-header bg-transparent border-0 p-0">
                <h6 class="text-muted">/etc/wireguard.conf</h6>
            </div>
            <div class="card-body p-0">
                <pre class="small"><% ex "cat /etc/wireguard.conf" %></pre>
            </div>
            <div class="d-flex justify-content-between align-items-center">
                <a class="btn btn-secondary mt-2" href="fw-editor.cgi?f=<%= /etc/wireguard.conf %>">Edit Wireguard Configuration</a>
                <form action="<%= $SCRIPT_NAME %>" method="post">
                    <input type="hidden" name="action" value="reload">
                    <% button_submit "Reload Wireguard Configuration" "success" %>
                </form>
            </div>
        </div>

        <div class="card border-0">
            <div class="card-header bg-transparent border-0 p-0">
                <h6 class="text-muted">/etc/network/interfaces.d/wg0</h6>
            </div>
            <div class="card-body p-0">
                <pre class="small"><% ex "cat /etc/network/interfaces.d/wg0" %></pre>
            </div>
            <div class="row">
                <p><a class="btn btn-secondary mt-2" href="fw-editor.cgi?f=<%= /etc/network/interfaces.d/wg0 %>">Edit Interface</a></p>
            </div>
        </div>
    </div>
</div>

<%in p/footer.cgi %>