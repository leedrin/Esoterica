#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    constexpr static float const g_playbackBarMinimumWidth = 120;
    constexpr static float const g_playbackBarHeight = 10;
    constexpr static float const g_playbackBarMarkerSize = 4;
    constexpr static float const g_playbackBarRegionHeight = g_playbackBarHeight + g_playbackBarMarkerSize;

    EE::StringID const FlowToolsNode::s_pinTypes[] =
    {
        StringID( GetNameForValueType( GraphValueType::Unknown ) ),
        StringID( GetNameForValueType( GraphValueType::Bool ) ),
        StringID( GetNameForValueType( GraphValueType::ID ) ),
        StringID( GetNameForValueType( GraphValueType::Float ) ),
        StringID( GetNameForValueType( GraphValueType::Vector ) ),
        StringID( GetNameForValueType( GraphValueType::Target ) ),
        StringID( GetNameForValueType( GraphValueType::BoneMask ) ),
        StringID( GetNameForValueType( GraphValueType::Pose ) ),
        StringID( GetNameForValueType( GraphValueType::Special ) )
    };

    GraphValueType FlowToolsNode::GetValueTypeForPinType( StringID pinType )
    {
        for ( uint8_t i = 0; i <= (uint8_t) GraphValueType::Special; i++ )
        {
            if ( s_pinTypes[i] == pinType )
            {
                return (GraphValueType) i;
            }
        }

        EE_UNREACHABLE_CODE();
        return GraphValueType::Unknown;
    }

    //-------------------------------------------------------------------------

    void DrawPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float canvasWidth, PoseNodeDebugInfo const* pDebugInfo )
    {
        float const availableCanvasWidth = Math::Max( canvasWidth, g_playbackBarMinimumWidth );
        ImVec2 const playbackBarSize = ctx.CanvasToWindow( ImVec2( availableCanvasWidth, g_playbackBarHeight ) );
        ImVec2 const playbackBarTopLeft = ImGui::GetCursorScreenPos();
        ImVec2 const playbackBarBottomRight = playbackBarTopLeft + playbackBarSize;

        // Draw spacer
        //-------------------------------------------------------------------------

        ImGui::Dummy( playbackBarSize );

        // Draw Info
        //-------------------------------------------------------------------------

        if ( pDebugInfo != nullptr )
        {
            Percentage const percentageThroughTrack = pDebugInfo->m_currentTime.GetNormalizedTime();
            float const pixelOffsetForPercentageThrough = Math::Floor( playbackBarSize.x * percentageThroughTrack );

            // Draw events
            bool useAlternateColor = false;
            ImVec2 eventTopLeft = playbackBarTopLeft;
            ImVec2 eventBottomRight = playbackBarBottomRight;
            for ( auto const& evt : pDebugInfo->m_pSyncTrack->GetEvents() )
            {
                eventBottomRight.x = eventTopLeft.x + Math::Floor( playbackBarSize.x * evt.m_duration );
                ctx.m_pDrawList->AddRectFilled( eventTopLeft, eventBottomRight, useAlternateColor ? Colors::White : Colors::DarkGray );
                eventTopLeft.x = eventBottomRight.x;
                useAlternateColor = !useAlternateColor;
            }

            // Draw progress bar
            ImVec2 progressBarTopLeft = playbackBarTopLeft;
            ImVec2 progressBarBottomRight = playbackBarTopLeft + ImVec2( pixelOffsetForPercentageThrough, playbackBarSize.y );
            ctx.m_pDrawList->AddRectFilled( progressBarTopLeft, progressBarBottomRight, Colors::LimeGreen.GetAlphaVersion( 0.65f ) );

            // Draw Marker
            float const scaledMarkerSize = ctx.CanvasToWindow( g_playbackBarMarkerSize );
            ImVec2 t0( progressBarTopLeft.x + pixelOffsetForPercentageThrough, playbackBarBottomRight.y );
            ImVec2 t1( t0.x - scaledMarkerSize, playbackBarBottomRight.y + scaledMarkerSize );
            ImVec2 t2( t0.x + scaledMarkerSize, playbackBarBottomRight.y + scaledMarkerSize );
            ctx.m_pDrawList->AddLine( t0, t0 - ImVec2( 0, playbackBarSize.y ), Colors::LimeGreen );
            ctx.m_pDrawList->AddTriangleFilled( t0, t1, t2, Colors::LimeGreen );

            // Draw text info
            ImGui::Text( "Time: %.2f/%.2fs", pDebugInfo->m_currentTime.ToFloat() * pDebugInfo->m_duration, pDebugInfo->m_duration.ToFloat() );
            ImGui::Text( "Percent: %.1f%%", pDebugInfo->m_currentTime.ToFloat() * 100 );
            ImGui::Text( "Event: %d, %.1f%%", pDebugInfo->m_currentSyncTime.m_eventIdx, pDebugInfo->m_currentSyncTime.m_percentageThrough.ToFloat() * 100 );
            StringID const eventID = pDebugInfo->m_pSyncTrack->GetEventID( pDebugInfo->m_currentSyncTime.m_eventIdx );
            ImGui::Text( "Event ID: %s", eventID.IsValid() ? eventID.c_str() : "No ID");
        }
        else
        {
            // Draw empty playback visualization bar
            ctx.m_pDrawList->AddRectFilled( playbackBarTopLeft, playbackBarTopLeft + playbackBarSize, Colors::DarkGray );

            // Draw text placeholders
            ImGui::Text( "Time: N/A" );
            ImGui::Text( "Percent: N/A" );
            ImGui::Text( "Event: N/A" );
            ImGui::Text( "Event ID: N/A" );
        }
    }

    void DrawRuntimeNodeIndex( VisualGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, VisualGraph::BaseNode* pNode, int16_t runtimeNodeIdx )
    {
        ImGuiStyle const& style = ImGui::GetStyle();

        //-------------------------------------------------------------------------

        InlineString const idxStr( InlineString::CtorSprintf(), "%d", runtimeNodeIdx );

        ImVec2 const textSize = ImGui::CalcTextSize( idxStr.c_str() );
        float const scaledBubblePadding = ctx.CanvasToWindow( 4.0f );
        float const scaledVerticalOffset = ctx.CanvasToWindow( pNode->GetNodeMargin() ).y + style.ItemSpacing.y;
        float const scaledBubbleWidth = textSize.x + style.ItemSpacing.x + ( scaledBubblePadding * 2 );
        float const scaledBubbleHeight = ImGui::GetFrameHeight();

        //-------------------------------------------------------------------------

        Float2 const nodeWindowPosition = ctx.CanvasToWindowPosition( pNode->GetPosition() );
        ImVec2 const startRect = ctx.WindowToScreenPosition( ImVec2( nodeWindowPosition.m_x, nodeWindowPosition.m_y - scaledBubbleHeight - scaledVerticalOffset ) );
        ImVec2 const endRect = ImVec2( startRect.x + scaledBubbleWidth, startRect.y + scaledBubbleHeight );
        float const scaledRounding = ctx.CanvasToWindow( 3.0f * ctx.m_viewScaleFactor );
        ctx.m_pDrawList->AddRectFilled( startRect, endRect, Colors::MediumRed, scaledRounding );

        //-------------------------------------------------------------------------

        auto pFont = ImGuiX::GetFont( ImGuiX::Font::Medium );
        Float2 const scaledTextOffset( scaledBubblePadding, scaledBubblePadding );
        ctx.m_pDrawList->AddText( pFont, pFont->FontSize * ctx.m_viewScaleFactor, startRect + scaledTextOffset, Colors::White, idxStr.c_str() );
    }

    void DrawVectorInfoText( VisualGraph::DrawContext const& ctx, Vector const& value )
    {
        ImGui::Text( "X: %.2f, Y: %.2f, Z: %.2f, W: %.2f", value.GetX(), value.GetY(), value.GetZ(), value.GetW() );
    }

    void DrawTargetInfoText( VisualGraph::DrawContext const& ctx, Target const& value )
    {
        if ( value.IsTargetSet() )
        {
            if ( value.IsBoneTarget() )
            {
                if ( value.GetBoneID().IsValid() )
                {
                    ImGui::Text( "Value: %s", value.GetBoneID().c_str() );
                }
                else
                {
                    ImGui::Text( "Value: Invalid" );
                }
            }
            else
            {
                Transform const& transform = value.GetTransform();
                Vector const& translation = transform.GetTranslation();
                EulerAngles const angles = transform.GetRotation().ToEulerAngles();

                ImGui::Text( "Rot: X: %.3f, Y: %.3f, Z: %.3f", angles.m_x.ToDegrees().ToFloat(), angles.m_y.ToDegrees().ToFloat(), angles.m_z.ToDegrees().ToFloat() );
                ImGui::Text( "Trans: X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
                ImGui::Text( "Scl: %.3f", transform.GetScale() );
            }
        }
        else
        {
            ImGui::Text( "Not Set" );
        }
    }

    void DrawValueDisplayText( VisualGraph::DrawContext const& ctx, ToolsGraphUserContext* pGraphNodeContext, int16_t runtimeNodeIdx, GraphValueType valueType )
    {
        EE_ASSERT( pGraphNodeContext != nullptr );
        EE_ASSERT( pGraphNodeContext->HasDebugData() );
        EE_ASSERT( runtimeNodeIdx != InvalidIndex );

        //-------------------------------------------------------------------------

        switch ( valueType )
        {
            case GraphValueType::Bool:
            {
                auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<bool>( runtimeNodeIdx );
                ImGui::Text( "Value: " );
                ImGui::SameLine();
                ImGui::TextColored( ( value ? Colors::LimeGreen : Colors::IndianRed ).ToFloat4(), value ? "True" : "False" );
            }
            break;

            case GraphValueType::ID:
            {
                auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<StringID>( runtimeNodeIdx );
                if ( value.IsValid() )
                {
                    ImGui::Text( "Value: %s", value.c_str() );
                }
                else
                {
                    ImGui::Text( "Value: Invalid" );
                }
            }
            break;

            case GraphValueType::Float:
            {
                auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<float>( runtimeNodeIdx );
                ImGui::Text( "Value: %.3f", value );
            }
            break;

            case GraphValueType::Vector:
            {
                auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<Vector>( runtimeNodeIdx );
                DrawVectorInfoText( ctx, value );
            }
            break;

            case GraphValueType::Target:
            {
                auto const value = pGraphNodeContext->GetRuntimeNodeDebugValue<Target>( runtimeNodeIdx );
                DrawTargetInfoText( ctx, value );
            }
            break;

            default:
            break;
        }
    }

    static void TraverseHierarchy( VisualGraph::BaseNode const* pNode, TVector<VisualGraph::BaseNode const*>& nodePath )
    {
        EE_ASSERT( pNode != nullptr );
        nodePath.emplace_back( pNode );

        if ( pNode->HasParentGraph() && !pNode->GetParentGraph()->IsRootGraph() )
        {
            TraverseHierarchy( pNode->GetParentGraph()->GetParentNode(), nodePath );
        }
    }

    //-------------------------------------------------------------------------

    void FlowToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        int16_t const runtimeNodeIdx = isPreviewing ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        bool const isPreviewingAndValidRuntimeNodeIdx = isPreviewing && ( runtimeNodeIdx != InvalidIndex );

        //-------------------------------------------------------------------------
        // Runtime Index Info
        //-------------------------------------------------------------------------

        if ( pGraphNodeContext->m_showRuntimeIndices && isPreviewingAndValidRuntimeNodeIdx )
        {
            DrawRuntimeNodeIndex( ctx, pGraphNodeContext, this, runtimeNodeIdx );
        }

        //-------------------------------------------------------------------------
        // Draw Pose Node
        //-------------------------------------------------------------------------

        if ( GetValueType() == GraphValueType::Pose )
        {
            if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                PoseNodeDebugInfo const debugInfo = pGraphNodeContext->GetPoseNodeDebugInfo( runtimeNodeIdx );
                DrawPoseNodeDebugInfo( ctx, GetWidth(), &debugInfo );
            }
            else
            {
                DrawPoseNodeDebugInfo( ctx, GetWidth(), nullptr );
            }

            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

            DrawInfoText( ctx );
        }

        //-------------------------------------------------------------------------
        // Draw Value Node
        //-------------------------------------------------------------------------

        else
        {
            DrawInfoText( ctx );

            if ( GetValueType() != GraphValueType::Unknown && GetValueType() != GraphValueType::BoneMask && GetValueType() != GraphValueType::Pose && GetValueType() != GraphValueType::Special )
            {
                DrawInternalSeparator( ctx, s_genericNodeSeparatorColor );

                BeginDrawInternalRegion( ctx );

                if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) && HasOutputPin() )
                {
                    DrawValueDisplayText( ctx, pGraphNodeContext, runtimeNodeIdx, GetValueType() );
                }
                else
                {
                    ImGui::NewLine();
                }

                EndDrawInternalRegion( ctx );
            }
        }
    }

    bool FlowToolsNode::IsActive( VisualGraph::UserContext* pUserContext ) const
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            // Some nodes dont have runtime representations
            auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                return pGraphNodeContext->IsNodeActive( runtimeNodeIdx );
            }
        }

        return false;
    }

    Color FlowToolsNode::GetTitleBarColor() const
    {
        return GetColorForValueType( GetValueType() );
    }

    Color FlowToolsNode::GetPinColor( VisualGraph::Flow::Pin const& pin ) const
    {
        return GetColorForValueType( GetValueTypeForPinType( pin.m_type ) );
    }

    void FlowToolsNode::DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin )
    {
        // Draw runtime node index
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            if ( ImGui::BeginMenu( EE_ICON_INFORMATION_OUTLINE" Runtime Node ID" ) )
            {
                int16_t runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
                if ( runtimeNodeIdx != InvalidIndex )
                {
                    InlineString const label( InlineString::CtorSprintf(), "Runtime Index: %d", runtimeNodeIdx );
                    if ( ImGui::MenuItem( label.c_str() ) )
                    {
                        InlineString const value( InlineString::CtorSprintf(), "%d", runtimeNodeIdx );
                        ImGui::SetClipboardText( value.c_str() );
                    }
                }

                ImGui::EndMenu();
            }
        }
    }
}